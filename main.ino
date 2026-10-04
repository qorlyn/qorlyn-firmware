/* ============================================================================
 * G-Mesh Project v2 - (C) 2026 Emanuele Ferraro & GeoReport International Tech
 * ML-KEM (Kyber-512) + AES-256-GCM (con AAD) + Mesh relay + Trasporto affidabile
 * ----------------------------------------------------------------------------
 * Architettura:
 *   - Un solo RadioTask possiede la radio; tutti gli altri inviano su coda.
 *   - Sessioni crypto per-peer (map peerID -> shared_secret + seq anti-replay).
 *   - Header versionato + flag; AAD autentica l'header (tranne TTL).
 *   - Relay flooding controllato con dedup LRU (32 voci) a livello frame.
 *   - Trasporto stop-and-wait per messaggio: ACK con bitmap, retry fino a 3.
 *   - FEC dinamico reale: interferenza -> CR LoRa 5..8.
 *   - Wake/sleep con persistenza dello stato in RTC_SLOW_MEM.
 * ==========================================================================*/

#include <Arduino.h>
#include <esp_mac.h>
#include <esp_sleep.h>
#include <esp_attr.h>
#include "mbedtls/gcm.h"
#include <RadioLib.h>
#include <SPI.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "pqc_kyber.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

// ============================================================================
// 1. CONFIGURAZIONE HARDWARE
// ============================================================================
#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RST 21
#define OLED_PWR 36
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define LORA_CS     8
#define LORA_DIO1   14
#define LORA_RST    12
#define LORA_BUSY   13
#define LORA_SCK    9
#define LORA_MISO   11
#define LORA_MOSI   10

#define BUTTON_PIN      GPIO_NUM_0
#define RADIO_DIO1_PIN  GPIO_NUM_14

SPIClass spi(FSPI);
SPISettings spiSettings(100000, MSBFIRST, SPI_MODE0);
SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY, spi, spiSettings);

// ============================================================================
// 2. COSTANTI DI PROTOCOLLO
// ============================================================================
#define G_VERSION            0x02
#define STX_BYTE             0x02
#define ETX_BYTE             0x03

#define DEFAULT_TTL          3
#define MAX_FRAMES           8
#define FRAME_PAYLOAD        128
#define MAX_MESSAGE_BYTES    (MAX_FRAMES * FRAME_PAYLOAD)   // 1024
#define BROADCAST_ID         0xFFFFFFFFu

#define CACHE_SIZE           32
#define MAX_PEERS            8
#define MAX_OUTGOING         3
#define ACK_TIMEOUT_MS       3000
#define ACK_MAX_RETRIES      3
#define RX_SLOT_COUNT        6
#define RX_SESSION_TIMEOUT   120000UL

#define KYBER_PK_SIZE        800
#define KYBER_SK_SIZE        1632
#define KYBER_CT_SIZE        768

#define WIFI_SSID_PREFIX     "G-TALK-"
#define WIFI_PASSWORD        "gmesh2026"
#define WEB_USER             "admin"
#define WEB_PASS             "gmesh2026"

uint16_t g_messagesReceived = 0; // +1 per ogni messaggio ricevuto, se arriva a me +1, esclusi ack e pair requests
// ============================================================================
// 3. TIPI DI MESSAGGIO
// ============================================================================
enum GMessageType : uint8_t {
    MSG_NET_TEST        = 0x00,
    MSG_HEARTBEAT       = 0x01,
    MSG_ACK             = 0x08,
    MSG_SAT_QUERY       = 0x09,
    MSG_NODE_QUERY      = 0x0A,
    MSG_BATT_LOW        = 0x0B,
    MSG_NET_TEST_ALT    = 0x17,
    MSG_NO_SIGNAL       = 0x16,
    MSG_IS_NODE         = 0x19,
    MSG_NACK            = 0x1B,

    MSG_SOS_GENERIC     = 0x02,
    MSG_SOS_MEDIC       = 0x03,
    MSG_CRIT_INJURY     = 0x04,
    MSG_LOST            = 0x15,
    MSG_GENERIC_PROBLEM = 0x12,
    MSG_PATH_BLOCKED    = 0x13,

    MSG_SUPPLY_LOW      = 0x05,
    MSG_STATUS_OK       = 0x06,
    MSG_FOUND           = 0x07,
    MSG_WEATHER_BAD     = 0x0C,
    MSG_RETURNING       = 0x0D,
    MSG_STATUS_FINE     = 0x18,
    MSG_AUTH_NOTIFIED   = 0x1A,

    MSG_HOW_ARE_YOU     = 0x0E,
    MSG_YES             = 0x0F,
    MSG_NO              = 0x10,
    MSG_DONT_KNOW       = 0x11,
    MSG_RECEIVED        = 0x14,
    MSG_OK              = 0x1D,

    MSG_CHAT            = 0x1C,
    MSG_KYBER_PUBKEY    = 0x20,
    MSG_KYBER_CIPHERTEXT= 0x21
};

enum PacketFlags : uint8_t {
    FLAG_ENCRYPTED  = 0x01,
    FLAG_ACK_REQ    = 0x02,
    FLAG_RELAY_OK   = 0x04,
    FLAG_BROADCAST  = 0x08
};

// ============================================================================
// 4. STRUTTURE
// ============================================================================
struct __attribute__((packed)) GPacket {
    uint8_t  stx;
    uint8_t  version;
    uint8_t  flags;
    uint32_t senderID;
    uint32_t targetID;
    uint16_t sessionID;
    uint8_t  msgType;
    uint8_t  ttl;
    uint8_t  frameIdx;
    uint8_t  totalFrames;
    uint16_t payloadLen;
    uint32_t timestamp;
    uint32_t seq;
    uint8_t  payload[FRAME_PAYLOAD];
    uint8_t  etx;
};

struct PeerSession {
    bool     inUse;
    uint32_t peerID;
    bool     established;
    uint8_t  shared_secret[32];
    uint32_t last_rx_seq;
    uint32_t tx_seq;
    unsigned long last_activity;
};

struct IncomingMessage {
    bool     active;
    uint32_t senderID;
    uint32_t targetID;
    uint16_t sessionID;
    uint8_t  msgType;
    uint8_t  totalFrames;
    uint8_t  receivedCount;
    uint8_t  frameMask;
    uint16_t totalPayloadLen;
    uint8_t  data[MAX_MESSAGE_BYTES];
    unsigned long lastUpdate;
};

struct OutgoingMessage {
    bool     active;
    uint32_t targetID;
    uint16_t sessionID;
    uint8_t  msgType;
    uint8_t  flags;
    uint8_t  totalFrames;
    uint8_t  ackedMask;
    uint8_t  retries;
    uint16_t frameLens[MAX_FRAMES];
    uint8_t  frameData[MAX_FRAMES][FRAME_PAYLOAD];
    unsigned long lastSend;
};

struct TxRequest {
    GPacket packet;
};

// ============================================================================
// 5. GLOBALI
// ============================================================================
RTC_DATA_ATTR uint32_t g_sessionCounter = 0;
RTC_DATA_ATTR uint32_t g_myID           = 0;
RTC_DATA_ATTR uint8_t  g_kyber_pk[KYBER_PK_SIZE];
RTC_DATA_ATTR uint8_t  g_kyber_sk[KYBER_SK_SIZE];
RTC_DATA_ATTR bool     g_kyber_ready    = false;

uint32_t myID = 0;
SemaphoreHandle_t stateMutex = nullptr;
QueueHandle_t     txQueue    = nullptr;
TaskHandle_t      radioTaskHandle = nullptr;

Adafruit_SSD1306  oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
AsyncWebServer    server(80);

PeerSession      peers[MAX_PEERS];
IncomingMessage  rxSlots[RX_SLOT_COUNT];
OutgoingMessage  outgoing[MAX_OUTGOING];

uint32_t dedupCache[CACHE_SIZE];
uint8_t  dedupIdx = 0;

volatile uint32_t foreign_packets_count = 0;
volatile uint32_t crc_error_count       = 0;
volatile uint8_t  interference_level    = 1;

unsigned long lastActivity = 0;

// ============================================================================
// 6. UTILITY
// ============================================================================
static inline uint32_t nowMs() { return millis(); }

static uint8_t DynamicFEC(uint8_t lvl);
static void updateUI(const char* status, const char* info, float rssi, float snr);

static void updateUI(const char* status, const char* info, float rssi = 0, float snr = 0) {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);

    oled.setCursor(0, 0);
    oled.printf("ID: %08X", myID);

    oled.setCursor(0, 10);
    oled.print("Sec: ");
    bool anySecure = false;
    for (int i = 0; i < MAX_PEERS; i++) {
        if (peers[i].inUse && peers[i].established) { anySecure = true; break; }
    }
    oled.print(anySecure ? "KYBER+GCM" : "OFF");

    oled.setCursor(0, 22);
    oled.printf("FEC:%u INT:%u", DynamicFEC(interference_level), interference_level);

    if (rssi != 0) {
        oled.setCursor(0, 34);
        oled.printf("Sig %.1f dBm SNR %.1f", rssi, snr);
    }

    oled.setCursor(0, 46);
    oled.print(status);
    oled.setCursor(0, 56);
    oled.print(info);
    oled.display();
}

static uint8_t DynamicFEC(uint8_t lvl) {
    if (lvl <= 2)  return 5;
    if (lvl <= 5)  return 6;
    if (lvl <= 8)  return 7;
    return 8;
}

static void recalcInterference() {
    uint32_t lvl = (foreign_packets_count / 10) + (crc_error_count / 5);
    if (lvl > 10) lvl = 10;
    if (lvl < 1)  lvl = 1;
    interference_level = (uint8_t)lvl;
}

static uint32_t generateDeviceID() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    uint32_t id = ((uint32_t)mac[2] << 24) | ((uint32_t)mac[3] << 16)
                | ((uint32_t)mac[4] << 8)  | (uint32_t)mac[5];
    if (id == BROADCAST_ID) id ^= 0x1;
    return id;
}

// ============================================================================
// 7. DEDUP CACHE
// ============================================================================
static bool dedupCheckAndInsert(uint32_t sender, uint16_t session, uint8_t frameIdx) {
    uint32_t h = sender ^ ((uint32_t)session << 13) ^ ((uint32_t)frameIdx * 2654435761u);
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (dedupCache[i] == h) return true;
    }
    dedupCache[dedupIdx] = h;
    dedupIdx = (dedupIdx + 1) % CACHE_SIZE;
    return false;
}

// ============================================================================
// 8. PEER SESSIONS
// ============================================================================
static PeerSession* peerFind(uint32_t id) {
    for (int i = 0; i < MAX_PEERS; i++) {
        if (peers[i].inUse && peers[i].peerID == id) return &peers[i];
    }
    return nullptr;
}

static PeerSession* peerGetOrCreate(uint32_t id) {
    PeerSession* p = peerFind(id);
    if (p) return p;
    for (int i = 0; i < MAX_PEERS; i++) {
        if (!peers[i].inUse) {
            memset(&peers[i], 0, sizeof(PeerSession));
            peers[i].inUse = true;
            peers[i].peerID = id;
            peers[i].last_activity = nowMs();
            return &peers[i];
        }
    }
    int oldest = 0;
    for (int i = 1; i < MAX_PEERS; i++) {
        if (peers[i].last_activity < peers[oldest].last_activity) oldest = i;
    }
    memset(&peers[oldest], 0, sizeof(PeerSession));
    peers[oldest].inUse = true;
    peers[oldest].peerID = id;
    peers[oldest].last_activity = nowMs();
    return &peers[oldest];
}

static void peersSweep() {
    unsigned long t = nowMs();
    for (int i = 0; i < MAX_PEERS; i++) {
        if (peers[i].inUse && (t - peers[i].last_activity > 30UL * 60UL * 1000UL)) {
            memset(&peers[i], 0, sizeof(PeerSession));
        }
    }
}

// ============================================================================
// 9. CRITTOGRAFIA AES-256-GCM con AAD
// ============================================================================
#define ENC_OVERHEAD 28

static void buildAAD(const GPacket& p, uint8_t* aad, size_t& aadLen) {
    size_t i = 0;
    aad[i++] = p.version;
    aad[i++] = p.flags;
    memcpy(aad + i, &p.senderID, 4);    i += 4;
    memcpy(aad + i, &p.targetID, 4);    i += 4;
    memcpy(aad + i, &p.sessionID, 2);   i += 2;
    aad[i++] = p.msgType;
    aad[i++] = p.frameIdx;
    aad[i++] = p.totalFrames;
    memcpy(aad + i, &p.payloadLen, 2);  i += 2;
    memcpy(aad + i, &p.timestamp, 4);   i += 4;
    memcpy(aad + i, &p.seq, 4);         i += 4;
    aadLen = i;
}

static bool encryptGCM_AAD(const uint8_t* key,
                           const GPacket& p,
                           const uint8_t* plaintext, uint16_t len,
                           uint8_t* outPayload, uint16_t& outLen) {
    if (len + ENC_OVERHEAD > FRAME_PAYLOAD) return false;

    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);
    if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 256) != 0) {
        mbedtls_gcm_free(&gcm);
        return false;
    }

    uint8_t iv[12];
    for (int i = 0; i < 12; i++) iv[i] = (uint8_t)esp_random();
    memcpy(outPayload, iv, 12);

    uint8_t aad[64]; size_t aadLen = 0;
    buildAAD(p, aad, aadLen);

    uint8_t tag[16];
    int r = mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, len,
                                      iv, 12, aad, aadLen,
                                      plaintext, outPayload + ENC_OVERHEAD,
                                      16, tag);
    memcpy(outPayload + 12, tag, 16);
    mbedtls_gcm_free(&gcm);

    if (r != 0) return false;
    outLen = len + ENC_OVERHEAD;
    return true;
}

static bool decryptGCM_AAD(const uint8_t* key,
                           const GPacket& p,
                           const uint8_t* inPayload, uint16_t inLen,
                           uint8_t* outPlain, uint16_t& outLen) {
    if (inLen < ENC_OVERHEAD) return false;
    uint16_t cipherLen = inLen - ENC_OVERHEAD;

    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);
    if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 256) != 0) {
        mbedtls_gcm_free(&gcm);
        return false;
    }

    uint8_t iv[12], tag[16];
    memcpy(iv,  inPayload, 12);
    memcpy(tag, inPayload + 12, 16);

    uint8_t aad[64]; size_t aadLen = 0;
    buildAAD(p, aad, aadLen);

    int r = mbedtls_gcm_auth_decrypt(&gcm, cipherLen,
                                     iv, 12, aad, aadLen,
                                     tag, 16,
                                     inPayload + ENC_OVERHEAD, outPlain);
    mbedtls_gcm_free(&gcm);
    if (r != 0) return false;
    outLen = cipherLen;
    return true;
}

// ============================================================================
// 10. KYBER
// ============================================================================
static void kyberEnsureKeypair() {
    if (g_kyber_ready) return;
    Serial.println("[KYBER] Generazione keypair...");
    pqc_kyber512_keypair(g_kyber_pk, g_kyber_sk);
    g_kyber_ready = true;
    Serial.println("[KYBER] Pronto.");
}

// ============================================================================
// 11. RADIO TASK
// ============================================================================
static void radioTransmitBlocking(const GPacket& p) {
    uint32_t start = nowMs();
    while (digitalRead(LORA_BUSY) == HIGH && (nowMs() - start) < 500) delay(1);

    radio.standby();
    delay(20);

    int st = radio.transmit((uint8_t*)&p, sizeof(GPacket));
    if (st != RADIOLIB_ERR_NONE) {
        Serial.printf("[TX] err=%d\n", st);
    }
    delay(20);
    radio.startReceive();
}

static void handleReceivedPacket(GPacket& p, float rssi, float snr);

static void radioTask(void* /*arg*/) {
    TxRequest req;

    for (;;) {
        while (xQueueReceive(txQueue, &req, 0) == pdTRUE) {
            radioTransmitBlocking(req.packet);
        }

        GPacket p;
        int st = radio.receive((uint8_t*)&p, sizeof(GPacket), 100);
        if (st == RADIOLIB_ERR_NONE) {
            if (p.stx == STX_BYTE && p.etx == ETX_BYTE && p.version == G_VERSION) {
                float rssi = radio.getRSSI();
                float snr  = radio.getSNR();
                handleReceivedPacket(p, rssi, snr);
            } else {
                crc_error_count++;
            }
        } else if (st == RADIOLIB_ERR_CRC_MISMATCH) {
            crc_error_count++;
        }
        recalcInterference();

        // Cede CPU ai task IDLE: evita il task watchdog su IDLE0/IDLE1
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

static bool enqueueTx(const GPacket& p) {
    TxRequest r; r.packet = p;
    return xQueueSend(txQueue, &r, pdMS_TO_TICKS(500)) == pdTRUE;
}

// ============================================================================
// 12. COSTRUZIONE PACCHETTO
// ============================================================================
static GPacket buildHeader(GMessageType type, uint32_t target,
                           uint8_t totalFrames, uint8_t frameIdx,
                           uint16_t payloadLen, uint8_t flags,
                           uint16_t sessionID, uint32_t seq,
                           uint8_t ttl = DEFAULT_TTL) {
    GPacket p;
    memset(&p, 0, sizeof(p));
    p.stx         = STX_BYTE;
    p.version     = G_VERSION;
    p.flags       = flags;
    p.senderID    = myID;
    p.targetID    = target;
    p.sessionID   = sessionID;
    p.msgType     = (uint8_t)type;
    p.ttl         = ttl;
    p.frameIdx    = frameIdx;
    p.totalFrames = totalFrames;
    p.payloadLen  = payloadLen;
    p.timestamp   = nowMs() / 1000;
    p.seq         = seq;
    p.etx         = ETX_BYTE;
    return p;
}

// ============================================================================
// 13. OUTGOING / ACK / RETRY
// ============================================================================
static OutgoingMessage* outgoingAlloc() {
    for (int i = 0; i < MAX_OUTGOING; i++) {
        if (!outgoing[i].active) return &outgoing[i];
    }
    return nullptr;
}

static void outgoingFree(OutgoingMessage* o) {
    if (o) memset(o, 0, sizeof(OutgoingMessage));
}

static bool sendFrameOfOutgoing(OutgoingMessage* o, uint8_t idx) {
    GPacket p = buildHeader((GMessageType)o->msgType, o->targetID,
                            o->totalFrames, idx,
                            o->frameLens[idx], o->flags,
                            o->sessionID, g_sessionCounter++,
                            DEFAULT_TTL);
    memcpy(p.payload, o->frameData[idx], o->frameLens[idx]);
    return enqueueTx(p);
}

static void outgoingStart(GMessageType type, uint32_t target,
                          const uint8_t* data, uint16_t len,
                          uint8_t extraFlags) {
    OutgoingMessage* o = outgoingAlloc();
    if (!o) {
        Serial.println("[TX] coda outgoing piena");
        return;
    }

    uint8_t totalFrames = (len == 0) ? 1 :
                          ((len + FRAME_PAYLOAD - 1) / FRAME_PAYLOAD);
    if (totalFrames > MAX_FRAMES) {
        Serial.println("[TX] messaggio troppo lungo");
        return;
    }

    memset(o, 0, sizeof(OutgoingMessage));
    o->active      = true;
    o->targetID    = target;
    o->sessionID   = ++g_sessionCounter;
    o->msgType     = (uint8_t)type;
    o->flags       = extraFlags | FLAG_ACK_REQ;
    if (target == BROADCAST_ID) o->flags |= FLAG_BROADCAST;
    o->totalFrames = totalFrames;
    o->retries     = 0;

    for (uint8_t i = 0; i < totalFrames; i++) {
        uint16_t off = i * FRAME_PAYLOAD;
        uint16_t l   = (len - off > FRAME_PAYLOAD) ? FRAME_PAYLOAD : (len - off);
        o->frameLens[i] = l;
        if (data) memcpy(o->frameData[i], data + off, l);
    }

    for (uint8_t i = 0; i < totalFrames; i++) sendFrameOfOutgoing(o, i);
    o->lastSend = nowMs();
}

static void outgoingHandleAck(uint32_t sender, uint16_t session, uint8_t ackMask) {
    for (int i = 0; i < MAX_OUTGOING; i++) {
        OutgoingMessage* o = &outgoing[i];
        if (!o->active) continue;
        if (o->sessionID != session) continue;
        if (o->targetID  != sender)  continue;

        o->ackedMask |= ackMask;
        uint8_t need = (1u << o->totalFrames) - 1;
        if ((o->ackedMask & need) == need) {
            Serial.printf("[TX] sessione %u completata\n", session);
            outgoingFree(o);
        }
    }
}

static void outgoingTick() {
    unsigned long t = nowMs();
    for (int i = 0; i < MAX_OUTGOING; i++) {
        OutgoingMessage* o = &outgoing[i];
        if (!o->active) continue;
        if (t - o->lastSend < ACK_TIMEOUT_MS) continue;

        uint8_t need = (1u << o->totalFrames) - 1;
        uint8_t missing = need & ~o->ackedMask;

        if (o->retries >= ACK_MAX_RETRIES) {
            Serial.printf("[TX] sessione %u timeout -> drop\n", o->sessionID);
            outgoingFree(o);
            continue;
        }

        o->retries++;
        Serial.printf("[TX] retry sessione %u (miss=0x%02X)\n", o->sessionID, missing);
        for (uint8_t f = 0; f < o->totalFrames; f++) {
            if (missing & (1u << f)) sendFrameOfOutgoing(o, f);
        }
        o->lastSend = t;
    }
}

// ============================================================================
// 14. RICEZIONE / RIASSEMBLAGGIO
// ============================================================================
static IncomingMessage* rxSlotFor(const GPacket& p, bool create) {
    for (int i = 0; i < RX_SLOT_COUNT; i++) {
        if (rxSlots[i].active &&
            rxSlots[i].senderID  == p.senderID &&
            rxSlots[i].sessionID == p.sessionID) return &rxSlots[i];
    }
    if (!create) return nullptr;
    for (int i = 0; i < RX_SLOT_COUNT; i++) {
        if (!rxSlots[i].active) {
            memset(&rxSlots[i], 0, sizeof(IncomingMessage));
            rxSlots[i].active       = true;
            rxSlots[i].senderID     = p.senderID;
            rxSlots[i].targetID     = p.targetID;
            rxSlots[i].sessionID    = p.sessionID;
            rxSlots[i].msgType      = p.msgType;
            rxSlots[i].totalFrames  = (p.totalFrames > MAX_FRAMES) ? MAX_FRAMES : p.totalFrames;
            rxSlots[i].lastUpdate   = nowMs();
            return &rxSlots[i];
        }
    }
    return nullptr;
}

static void rxSlotsSweep() {
    unsigned long t = nowMs();
    for (int i = 0; i < RX_SLOT_COUNT; i++) {
        if (rxSlots[i].active && (t - rxSlots[i].lastUpdate) > RX_SESSION_TIMEOUT) {
            Serial.printf("[RX] timeout sessione %u\n", rxSlots[i].sessionID);
            memset(&rxSlots[i], 0, sizeof(IncomingMessage));
        }
    }
}

static void processFinalMessage(GPacket& lastFrame, uint8_t* data, uint16_t len);

static void sendAckBitmap(uint32_t target, uint16_t session, uint8_t mask) {
    if (target == BROADCAST_ID) return;

    uint8_t payload[4];
    payload[0] = (session >> 8) & 0xFF;
    payload[1] = session & 0xFF;
    payload[2] = mask;
    payload[3] = 0;

    GPacket p = buildHeader(MSG_ACK, target, 1, 0, 4, 0,
                            ++g_sessionCounter, g_sessionCounter++, DEFAULT_TTL);
    memcpy(p.payload, payload, 4);
    enqueueTx(p);
}

static void handlePacketReassembly(GPacket& p) {
    IncomingMessage* slot = rxSlotFor(p, true);
    if (!slot) return;

    if (p.frameIdx < MAX_FRAMES) {
        uint8_t bit = 1u << p.frameIdx;
        if ((slot->frameMask & bit) == 0) {
            uint16_t off = p.frameIdx * FRAME_PAYLOAD;
            uint16_t l   = p.payloadLen;
            if (l > FRAME_PAYLOAD) l = FRAME_PAYLOAD;
            if (off + l <= MAX_MESSAGE_BYTES) {
                memcpy(slot->data + off, p.payload, l);
                slot->frameMask |= bit;
                slot->receivedCount++;
                slot->totalPayloadLen += l;
                slot->lastUpdate = nowMs();
            }
        }
    }

    uint8_t need = (1u << slot->totalFrames) - 1;
    if ((slot->frameMask & need) == need) {
        processFinalMessage(p, slot->data, slot->totalPayloadLen);
        if (p.targetID == myID && (p.flags & FLAG_ACK_REQ)) {
            sendAckBitmap(p.senderID, p.sessionID, slot->frameMask);
        }
        memset(slot, 0, sizeof(IncomingMessage));
    } else if (p.targetID == myID && (p.flags & FLAG_ACK_REQ)) {
        sendAckBitmap(p.senderID, p.sessionID, slot->frameMask);
    }
}

// ============================================================================
// 15. HANDLER DI MESSAGGI FINALI
// ============================================================================
static void handleKeyExchange(uint8_t type, const uint8_t* data, uint16_t len, uint32_t remoteID) {
    if (type == MSG_KYBER_PUBKEY) {
        if (len != KYBER_PK_SIZE) { Serial.println("[KYBER] PK size errata"); return; }
        uint8_t ct[KYBER_CT_SIZE];
        uint8_t shared[32];
        pqc_kyber512_encapsulate(ct, shared, data);

        PeerSession* ps = peerGetOrCreate(remoteID);
        memcpy(ps->shared_secret, shared, 32);
        ps->established = true;
        ps->tx_seq = 1;
        ps->last_rx_seq = 0;
        ps->last_activity = nowMs();

        outgoingStart(MSG_KYBER_CIPHERTEXT, remoteID, ct, KYBER_CT_SIZE, FLAG_RELAY_OK);
        Serial.printf("[KYBER] Sessione con %08X pronta (responder)\n", remoteID);
        updateUI("SECURE", "Handshake OK");
    }
    else if (type == MSG_KYBER_CIPHERTEXT) {
        if (len != KYBER_CT_SIZE) { Serial.println("[KYBER] CT size errata"); return; }
        if (!g_kyber_ready) { Serial.println("[KYBER] no keypair locale"); return; }
        uint8_t shared[32];
        pqc_kyber512_decapsulate(shared, data, g_kyber_sk);

        PeerSession* ps = peerGetOrCreate(remoteID);
        memcpy(ps->shared_secret, shared, 32);
        ps->established = true;
        ps->tx_seq = 1;
        ps->last_rx_seq = 0;
        ps->last_activity = nowMs();

        Serial.printf("[KYBER] Sessione con %08X pronta (initiator)\n", remoteID);
        updateUI("SECURE", "Handshake OK");
    }
}

static void processFinalMessage(GPacket& lastFrame, uint8_t* data, uint16_t len) {
    uint32_t sender = lastFrame.senderID;
    uint8_t  type   = lastFrame.msgType;

    if (type == MSG_KYBER_PUBKEY || type == MSG_KYBER_CIPHERTEXT) {
        handleKeyExchange(type, data, len, sender);
        return;
    }

    if (type == MSG_ACK) {
        if (len < 3) return;
        uint16_t sess = (data[0] << 8) | data[1];
        uint8_t  mask = data[2];
        outgoingHandleAck(sender, sess, mask);
        return;
    }
    if (type == MSG_NACK) {
        Serial.printf("[RX] NACK da %08X\n", sender);
        return;
    }
    if (type != MSG_ACK && type != MSG_KYBER_PUBKEY && type != MSG_HEARTBEAT && sender && forMe == true) {
        g_messagesReceived++;
    }

    const uint8_t* plain = data;
    uint16_t plainLen = len;
    static uint8_t tmp[MAX_MESSAGE_BYTES];

    if (lastFrame.flags & FLAG_ENCRYPTED) {
        PeerSession* ps = peerFind(sender);
        if (!ps || !ps->established) {
            Serial.printf("[RX] cifrato da peer sconosciuto %08X\n", sender);
            return;
        }
        uint16_t outLen = 0;
        if (!decryptGCM_AAD(ps->shared_secret, lastFrame, data, len, tmp, outLen)) {
            Serial.println("[RX] GCM auth fail");
            return;
        }
        if (lastFrame.seq <= ps->last_rx_seq) {
            Serial.println("[RX] replay respinto");
            return;
        }
        ps->last_rx_seq = lastFrame.seq;
        ps->last_activity = nowMs();

        plain = tmp;
        plainLen = outLen;
    }

    switch (type) {
        case MSG_HEARTBEAT:
            Serial.println("[RX] HB");
            break;
        case MSG_SOS_MEDIC:
            Serial.println("[RX] !!! SOS MEDICO !!!");
            updateUI("SOS", "SOS MEDICO");
            break;
        case MSG_CHAT: {
            char buf[MAX_MESSAGE_BYTES + 1];
            uint16_t n = plainLen > MAX_MESSAGE_BYTES ? MAX_MESSAGE_BYTES : plainLen;
            memcpy(buf, plain, n);
            buf[n] = '\0';
            Serial.printf("[RX] Chat da %08X: %s\n", sender, buf);
            updateUI("CHAT", buf);
            break;
        }
        default:
            Serial.printf("[RX] tipo 0x%02X len=%u\n", type, plainLen);
            break;
    }
}

// ============================================================================
// 16. INSTADRAMENTO RICEZIONE
// ============================================================================
static void handleReceivedPacket(GPacket& p, float rssi, float snr) {
    if (dedupCheckAndInsert(p.senderID, p.sessionID, p.frameIdx)) return;

    bool forMe     = (p.targetID == myID || p.targetID == BROADCAST_ID);
    bool relayable = (p.flags & FLAG_RELAY_OK) && p.ttl > 0 && p.senderID != myID;

    if (!forMe) foreign_packets_count++;

    if (relayable && !forMe) {
        GPacket r = p;
        r.ttl--;
        enqueueTx(r);
    }

    if (forMe) {
        updateUI("RX", "Msg in arrivo", rssi, snr);
        handlePacketReassembly(p);
    }

    lastActivity = nowMs();
}

// ============================================================================
// 17. INVIO DI ALTO LIVELLO
// ============================================================================
static void sendSecureOrPlain(GMessageType type, uint32_t target,
                              const uint8_t* data, uint16_t len) {
    PeerSession* ps = peerFind(target);
    bool doEnc = (ps && ps->established && target != BROADCAST_ID);

    uint8_t flags = FLAG_RELAY_OK;
    if (target == BROADCAST_ID) flags |= FLAG_BROADCAST;

    if (doEnc && len + ENC_OVERHEAD <= FRAME_PAYLOAD) {
        uint16_t sessTmp = g_sessionCounter + 1;
        uint32_t seqTmp  = ps->tx_seq;
        GPacket hdr = buildHeader(type, target, 1, 0, (uint16_t)len, flags,
                                  sessTmp, seqTmp, DEFAULT_TTL);
        uint8_t blob[FRAME_PAYLOAD];
        uint16_t blobLen = 0;
        if (encryptGCM_AAD(ps->shared_secret, hdr, data, len, blob, blobLen)) {
            ps->tx_seq++;
            flags |= FLAG_ENCRYPTED;
            outgoingStart(type, target, blob, blobLen, flags);
            return;
        }
        Serial.println("[TX] encrypt fail, invio in chiaro");
    } else if (doEnc) {
        Serial.println("[TX] payload troppo grande per cifratura singola, invio in chiaro");
    }

    outgoingStart(type, target, data, len, flags);
}

// ============================================================================
// 18. HEARTBEAT
// ============================================================================
static unsigned long lastHeartbeat = 0;
static void heartbeatTick() {
    if (nowMs() - lastHeartbeat < 60000) return;
    lastHeartbeat = nowMs();
    outgoingStart(MSG_HEARTBEAT, BROADCAST_ID, nullptr, 0, FLAG_RELAY_OK);
}

// ============================================================================
// 19. WEB UI
// ============================================================================
static bool webAuth(AsyncWebServerRequest* r) {
    if (!r->authenticate(WEB_USER, WEB_PASS)) {
        r->requestAuthentication();
        return false;
    }
    return true;
}

static void setupWeb() {
    char apName[24];
    sprintf(apName, "%s%08X", WIFI_SSID_PREFIX, myID);
    WiFi.softAP(apName, WIFI_PASSWORD);
    Serial.printf("[WiFi] AP %s -> %s\n", apName, WiFi.softAPIP().toString().c_str());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!webAuth(req)) return;
        lastActivity = nowMs();

        String h;
        h += "<!DOCTYPE HTML><html><head><meta charset='utf-8'>";
        h += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
        h += "<title>G-TALK</title><style>";
        h += "body{font-family:sans-serif;background:#181818;color:#eee;text-align:center;padding:12px}";
        h += "button{padding:14px 20px;margin:6px;border:none;border-radius:6px;font-size:16px;cursor:pointer}";
        h += ".sos{background:#d32f2f;color:#fff}.msg{background:#388e3c;color:#fff}.key{background:#fbc02d;color:#000}";
        h += "input{padding:10px;width:85%;margin:6px;border-radius:6px;border:none}";
        h += "</style></head><body>";
        char idbuf[16]; sprintf(idbuf, "%08X", myID);
        h += "<h2>G-TALK</h2><div>ID: "; h += idbuf; h += "</div>";
        h += "<input id='t' placeholder='Target ID (hex)' value='FFFFFFFF'>";
        h += "<input id='m' placeholder='Messaggio'>";
        h += "<br><button class='msg' onclick=\"s('CHAT')\">CHAT</button>";
        h += "<button class='sos' onclick=\"s('SOS')\">SOS MEDICO</button>";
        h += "<button class='key' onclick=\"s('KEY')\">HANDSHAKE KYBER</button>";
        h += "<script>function s(t){var tg=document.getElementById('t').value;";
        h += "fetch('/send',{method:'POST',";
        h += "headers:{'Content-Type':'application/x-www-form-urlencoded'},";
        h += "body:'type='+encodeURIComponent(t)+'&target='+encodeURIComponent(tg)+";
        h += "'&msg='+encodeURIComponent(document.getElementById('m').value)});}</script>";
        h += "</body></html>";
        req->send(200, "text/html", h);
    });

    server.on("/send", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (!webAuth(req)) return;
        lastActivity = nowMs();

        if (!req->hasParam("type", true) || !req->hasParam("target", true)) {
            req->send(400, "text/plain", "parametri mancanti");
            return;
        }
        String type   = req->getParam("type", true)->value();
        String tstr   = req->getParam("target", true)->value();
        String msg    = req->hasParam("msg", true) ? req->getParam("msg", true)->value() : "";

        char* endp = nullptr;
        uint32_t target = strtoul(tstr.c_str(), &endp, 16);
        if (!endp || *endp != '\0' || tstr.length() == 0) {
            req->send(400, "text/plain", "target non valido");
            return;
        }

        if (type == "SOS") {
            sendSecureOrPlain(MSG_SOS_MEDIC, target, nullptr, 0);
        } else if (type == "CHAT") {
            if (msg.length() > MAX_MESSAGE_BYTES) {
                req->send(413, "text/plain", "msg troppo lungo");
                return;
            }
            sendSecureOrPlain(MSG_CHAT, target, (const uint8_t*)msg.c_str(), msg.length());
        } else if (type == "KEY") {
            if (target == BROADCAST_ID) {
                req->send(400, "text/plain", "no broadcast handshake");
                return;
            }
            kyberEnsureKeypair();
            outgoingStart(MSG_KYBER_PUBKEY, target, g_kyber_pk, KYBER_PK_SIZE, FLAG_RELAY_OK);
        } else {
            req->send(400, "text/plain", "tipo sconosciuto");
            return;
        }
        req->send(200, "text/plain", "ok");
    });

    server.begin();
}

// ============================================================================
// 20. SERIAL COMMANDS
// ============================================================================
static void serialTick() {
    while (Serial.available()) {
        int c = Serial.read();
        switch (c) {
            case 'h': {
                const char* m = "Hello world";
                sendSecureOrPlain(MSG_CHAT, BROADCAST_ID, (const uint8_t*)m, strlen(m));
                break;
            }
            case 'k': {
                kyberEnsureKeypair();
                Serial.println("[KYBER] PK pronta");
                break;
            }
            case 's': {
                const char* m = "Secure hi";
                sendSecureOrPlain(MSG_CHAT, BROADCAST_ID, (const uint8_t*)m, strlen(m));
                break;
            }
            case 'i':
                Serial.printf("FEC=%u INT=%u foreign=%lu crc=%lu\n",
                              DynamicFEC(interference_level), interference_level,
                              (unsigned long)foreign_packets_count,
                              (unsigned long)crc_error_count);
                break;
            case 'p':
                for (int i = 0; i < MAX_PEERS; i++) {
                    if (peers[i].inUse) {
                        Serial.printf("peer %08X est=%d last_rx=%u\n",
                                      peers[i].peerID, peers[i].established,
                                      peers[i].last_rx_seq);
                    }
                }
                break;
        }
    }
}

// ============================================================================
// 21. SLEEP
// ============================================================================
static void enterSleep() {
    Serial.println("[SLEEP] entering deep sleep");
    updateUI("SLEEP", "bye");
    delay(200);
    oled.ssd1306_command(SSD1306_DISPLAYOFF);
    server.end();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    esp_sleep_enable_ext0_wakeup(BUTTON_PIN, 0);
    esp_sleep_enable_ext1_wakeup((1ULL << RADIO_DIO1_PIN), ESP_EXT1_WAKEUP_ANY_HIGH);
    esp_sleep_enable_timer_wakeup(60ULL * 1000000ULL);
    esp_deep_sleep_start();
}

static void printWakeReason() {
    esp_sleep_wakeup_cause_t r = esp_sleep_get_wakeup_cause();
    switch (r) {
        case ESP_SLEEP_WAKEUP_EXT0:  Serial.println("[BOOT] wake: button"); break;
        case ESP_SLEEP_WAKEUP_EXT1:  Serial.println("[BOOT] wake: LoRa DIO1"); break;
        case ESP_SLEEP_WAKEUP_TIMER: Serial.println("[BOOT] wake: timer"); break;
        default: Serial.printf("[BOOT] wake: %d\n", r); break;
    }
}

// ============================================================================
// 22. RADIO SETUP
// ============================================================================
static bool setupRadio() {
    Serial.println("[RADIO] init");
    spi.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

    pinMode(LORA_CS, OUTPUT);   digitalWrite(LORA_CS, HIGH);
    pinMode(LORA_RST, OUTPUT);  digitalWrite(LORA_RST, LOW); delay(100);
                                digitalWrite(LORA_RST, HIGH); delay(200);
    pinMode(LORA_BUSY, INPUT);
    delay(50);

    int st = radio.begin(868.0, 125.0, 9, DynamicFEC(interference_level), 0x12, 10, 8);
    if (st != RADIOLIB_ERR_NONE) {
        Serial.printf("[RADIO] begin err=%d\n", st);
        return false;
    }
    radio.setCRC(true);
    radio.setOutputPower(10);
    delay(50);
    radio.startReceive();
    Serial.println("[RADIO] ready");
    return true;
}

// ============================================================================
// 23. OLED SETUP
// ============================================================================
static void setupOLED() {
    pinMode(OLED_PWR, OUTPUT);
    digitalWrite(OLED_PWR, LOW);
    delay(300);

    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, LOW);  delay(100);
    digitalWrite(OLED_RST, HIGH); delay(200);

    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(100000);
    delay(100);

    oled.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, false);
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(0, 0);
    oled.println("G-Mesh v2 boot...");
    oled.display();
}

// ============================================================================
// 24. SETUP / LOOP
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("\n=== G-MESH v2 ===");
    Serial.printf("GPacket=%u, IncomingMsg=%u, OutgoingMsg=%u\n",
                  (unsigned)sizeof(GPacket),
                  (unsigned)sizeof(IncomingMessage),
                  (unsigned)sizeof(OutgoingMessage));

    myID = g_myID ? g_myID : generateDeviceID();
    g_myID = myID;
    Serial.printf("myID=%08X\n", myID);

    stateMutex = xSemaphoreCreateMutex();
    txQueue    = xQueueCreate(16, sizeof(TxRequest));

    memset(peers, 0, sizeof(peers));
    memset(rxSlots, 0, sizeof(rxSlots));
    memset(outgoing, 0, sizeof(outgoing));
    for (int i = 0; i < CACHE_SIZE; i++) dedupCache[i] = 0xDEADBEEF;

    setupOLED();
    printWakeReason();

    if (!setupRadio()) {
        updateUI("ERROR", "radio fail");
    }

    // Task radio: priorità 2, pinnato su CPU 1
    xTaskCreatePinnedToCore(radioTask, "radio", 4096, nullptr, 2, &radioTaskHandle, 1);

    if (!g_kyber_ready) {
        kyberEnsureKeypair();
    } else {
        Serial.println("[KYBER] keypair ripristinata da RTC");
    }

    setupWeb();

    lastActivity = nowMs();
    updateUI("READY", "online");
}

void loop() {
    if (nowMs() - lastActivity > 10UL * 60UL * 1000UL) {
        enterSleep();
    }

    outgoingTick();
    rxSlotsSweep();
    peersSweep();
    heartbeatTick();
    serialTick();
    updateUI("MSG: ", g_messagesReceived);

    delay(20);
}
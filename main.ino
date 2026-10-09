/* ============================================================================
 * (C) Qorlyn 2026. Last update: 12:50:48 04/10/2026 (dd/mm/yyyy) (Italian time)
 * ML-KEM (Kyber-512) + AES-256-GCM (con AAD) + Full Onion Routing (multi-frame)
 * ----------------------------------------------------------------------------
 * Connect with us:
 *   🌐 Website:  https://qorlyn.org
 *   📧 Email:    qorlyn@qorlyn.org
 *   💬 Forum:    https://forum.qorlyn.org
 *    @ Discord:  qorlyn.org/discord
 *    @ Telegram: qorlyn.org/telegram
 *    @ GitHub:   qorlyn.org/github
 *    Donate:     qorlyn.org/donate
 NOTE: THIS CODE IS NOT TESTED. 
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
// 1. HARDWARE
// ============================================================================
#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RST 21
#define OLED_PWR 32
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
// 2. PROTOCOLLO
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
#define MAX_PEERS            6
#define MAX_OUTGOING         3
#define ACK_TIMEOUT_MS       3000
#define ACK_MAX_RETRIES      3
#define RX_SLOT_COUNT        4
#define RX_SESSION_TIMEOUT   120000UL

#define KYBER_PK_SIZE        800
#define KYBER_SK_SIZE        1632
#define KYBER_CT_SIZE        768

// Onion
#define ONION_MAX_INTERMEDIATES 2
#define ONION_MAX_PATH          (ONION_MAX_INTERMEDIATES + 1)   // 3
#define ONION_LAYER_HDR         7                               // next(4)+type(1)+len(2)
#define ONION_LAYER_OVERHEAD    (ONION_LAYER_HDR + 28)          // +IV+tag
#define ONION_AAD_LEN           9                               // ver(1)+sender(4)+hop(4)
#define ONION_MAX_PAYLOAD       (MAX_MESSAGE_BYTES - ONION_LAYER_OVERHEAD * ONION_MAX_PATH)

#define WIFI_SSID_PREFIX     "G-TALK-"
#define WIFI_PASSWORD        "gmesh2026"
#define WEB_USER             "admin"
#define WEB_PASS             "gmesh2026"

// ============================================================================
// 3. TIPI
// ============================================================================
enum GMessageType : uint8_t {
    MSG_HEARTBEAT       = 0x01,
    MSG_SOS_MEDIC       = 0x03,
    MSG_ACK             = 0x08,
    MSG_NACK            = 0x1B,
    MSG_CHAT            = 0x1C,
    MSG_KYBER_PUBKEY    = 0x20,
    MSG_KYBER_CIPHERTEXT= 0x21,
    MSG_ONION           = 0x22
};

enum PacketFlags : uint8_t {
    FLAG_ENCRYPTED      = 0x01,
    FLAG_ACK_REQ        = 0x02,
    FLAG_RELAY_OK       = 0x04,
    FLAG_BROADCAST      = 0x08,
    FLAG_PLAINTEXT_WARN = 0x10,
    FLAG_NO_RETRY       = 0x20
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
    uint16_t sessionID;
    uint8_t  totalFrames;
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

struct TxRequest { GPacket packet; };

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

uint32_t dedupCache[CACHE_SIZE] = {0};
uint8_t  dedupIdx = 0;

volatile uint32_t foreign_packets_count = 0;
volatile uint32_t crc_error_count       = 0;
volatile uint8_t  interference_level    = 1;

unsigned long lastActivity = 0;
unsigned long g_plaintextWarnUntil = 0;

// Buffer statici per onion (protetti da stateMutex)
static uint8_t s_onionA[MAX_MESSAGE_BYTES];
static uint8_t s_onionB[MAX_MESSAGE_BYTES];
static uint8_t s_onionPlain[MAX_MESSAGE_BYTES];

// ============================================================================
// 6. UTILITY
// ============================================================================
static inline uint32_t nowMs() { return millis(); }

static uint8_t DynamicFEC(uint8_t lvl) {
    if (lvl <= 2)  return 5;
    if (lvl <= 5)  return 6;
    if (lvl <= 8)  return 7;
    return 8;
}

static void updateUI(const char* status, const char* info, float rssi = 0, float snr = 0) {
    static unsigned long lastDraw = 0;
    static char lastStatus[24] = {0};
    static char lastInfo[24] = {0};
    unsigned long t = nowMs();
    bool changed = (strcmp(status, lastStatus) != 0) || (strcmp(info, lastInfo) != 0);
    if (!changed && (t - lastDraw < 1000)) return;
    if (t - lastDraw < 200) return;
    lastDraw = t;
    strncpy(lastStatus, status, sizeof(lastStatus) - 1);
    strncpy(lastInfo, info, sizeof(lastInfo) - 1);
    lastStatus[sizeof(lastStatus)-1] = '\0';
    lastInfo[sizeof(lastInfo)-1] = '\0';

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
// 7. DEDUP
// ============================================================================
static bool dedupCheckAndInsert(uint32_t sender, uint16_t session, uint8_t frameIdx) {
    uint32_t h = sender ^ ((uint32_t)session << 13) ^ ((uint32_t)frameIdx * 2654435761u);
    for (int i = 0; i < CACHE_SIZE; i++) if (dedupCache[i] == h) return true;
    dedupCache[dedupIdx] = h;
    dedupIdx = (dedupIdx + 1) % CACHE_SIZE;
    return false;
}

// ============================================================================
// 8. PEER SESSIONS
// ============================================================================
static PeerSession* peerFind(uint32_t id) {
    for (int i = 0; i < MAX_PEERS; i++)
        if (peers[i].inUse && peers[i].peerID == id) return &peers[i];
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
    for (int i = 1; i < MAX_PEERS; i++)
        if (peers[i].last_activity < peers[oldest].last_activity) oldest = i;
    memset(&peers[oldest], 0, sizeof(PeerSession));
    peers[oldest].inUse = true;
    peers[oldest].peerID = id;
    peers[oldest].last_activity = nowMs();
    return &peers[oldest];
}

static void peersSweep() {
    if (xSemaphoreTakeRecursive(stateMutex, portMAX_DELAY) != pdTRUE) return;
    unsigned long t = nowMs();
    for (int i = 0; i < MAX_PEERS; i++)
        if (peers[i].inUse && (t - peers[i].last_activity > 30UL * 60UL * 1000UL))
            memset(&peers[i], 0, sizeof(PeerSession));
    xSemaphoreGiveRecursive(stateMutex);
}

// ============================================================================
// 9. CRITTOGRAFIA
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

static bool encryptGCM_AAD(const uint8_t* key, const GPacket& p,
                           const uint8_t* plaintext, uint16_t len,
                           uint8_t* outPayload, uint16_t& outLen) {
    if (len + ENC_OVERHEAD > FRAME_PAYLOAD) return false;
    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);
    if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 256) != 0) {
        mbedtls_gcm_free(&gcm); return false;
    }
    uint8_t iv[12];
    for (int i = 0; i < 12; i++) iv[i] = (uint8_t)esp_random();
    memcpy(outPayload, iv, 12);
    uint8_t aad[64]; size_t aadLen = 0;
    buildAAD(p, aad, aadLen);
    uint8_t tag[16];
    int r = mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, len,
                                      iv, 12, aad, aadLen,
                                      plaintext, outPayload + ENC_OVERHEAD, 16, tag);
    memcpy(outPayload + 12, tag, 16);
    mbedtls_gcm_free(&gcm);
    if (r != 0) return false;
    outLen = len + ENC_OVERHEAD;
    return true;
}

static bool decryptGCM_AAD(const uint8_t* key, const GPacket& p,
                           const uint8_t* inPayload, uint16_t inLen,
                           uint8_t* outPlain, uint16_t& outLen) {
    if (inLen < ENC_OVERHEAD) return false;
    uint16_t cipherLen = inLen - ENC_OVERHEAD;
    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);
    if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 256) != 0) {
        mbedtls_gcm_free(&gcm); return false;
    }
    uint8_t iv[12], tag[16];
    memcpy(iv,  inPayload, 12);
    memcpy(tag, inPayload + 12, 16);
    uint8_t aad[64]; size_t aadLen = 0;
    buildAAD(p, aad, aadLen);
    int r = mbedtls_gcm_auth_decrypt(&gcm, cipherLen, iv, 12, aad, aadLen,
                                     tag, 16, inPayload + ENC_OVERHEAD, outPlain);
    mbedtls_gcm_free(&gcm);
    if (r != 0) return false;
    outLen = cipherLen;
    return true;
}

// AES-GCM generico con AAD esplicito (per layer onion)
static bool gcmEncryptAAD(const uint8_t* key,
                          const uint8_t* aad, size_t aadLen,
                          const uint8_t* plaintext, uint16_t len,
                          uint8_t* out, uint16_t& outLen) {
    if (len + ENC_OVERHEAD > FRAME_PAYLOAD) return false;
    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);
    if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 256) != 0) {
        mbedtls_gcm_free(&gcm); return false;
    }
    uint8_t iv[12];
    for (int i = 0; i < 12; i++) iv[i] = (uint8_t)esp_random();
    memcpy(out, iv, 12);
    uint8_t tag[16];
    int r = mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, len,
                                      iv, 12, aad, aadLen,
                                      plaintext, out + ENC_OVERHEAD, 16, tag);
    memcpy(out + 12, tag, 16);
    mbedtls_gcm_free(&gcm);
    if (r != 0) return false;
    outLen = len + ENC_OVERHEAD;
    return true;
}

static bool gcmDecryptAAD(const uint8_t* key,
                          const uint8_t* aad, size_t aadLen,
                          const uint8_t* in, uint16_t inLen,
                          uint8_t* out, uint16_t& outLen) {
    if (inLen < ENC_OVERHEAD) return false;
    uint16_t cipherLen = inLen - ENC_OVERHEAD;
    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);
    if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 256) != 0) {
        mbedtls_gcm_free(&gcm); return false;
    }
    uint8_t iv[12], tag[16];
    memcpy(iv, in, 12);
    memcpy(tag, in + 12, 16);
    int r = mbedtls_gcm_auth_decrypt(&gcm, cipherLen, iv, 12, aad, aadLen,
                                     tag, 16, in + ENC_OVERHEAD, out);
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
    Serial.println("[KYBER] keypair...");
    pqc_kyber512_keypair(g_kyber_pk, g_kyber_sk);
    g_kyber_ready = true;
    Serial.println("[KYBER] ready.");
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
    if (st != RADIOLIB_ERR_NONE) Serial.printf("[TX] err=%d\n", st);
    delay(20);
    radio.startReceive();
}

static void handleReceivedPacket(GPacket& p, float rssi, float snr);

static void radioTask(void*) {
    TxRequest req;
    for (;;) {
        while (xQueueReceive(txQueue, &req, 0) == pdTRUE)
            radioTransmitBlocking(req.packet);

        GPacket p;
        int st = radio.receive((uint8_t*)&p, sizeof(GPacket), 100);
        if (st == RADIOLIB_ERR_NONE) {
            if (p.stx == STX_BYTE && p.etx == ETX_BYTE && p.version == G_VERSION) {
                handleReceivedPacket(p, radio.getRSSI(), radio.getSNR());
            } else {
                crc_error_count++;
            }
        } else if (st == RADIOLIB_ERR_CRC_MISMATCH) {
            crc_error_count++;
        }
        recalcInterference();
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

static bool enqueueTx(const GPacket& p) {
    TxRequest r; r.packet = p;
    return xQueueSend(txQueue, &r, pdMS_TO_TICKS(500)) == pdTRUE;
}

// ============================================================================
// 12. HEADER
// ============================================================================
static GPacket buildHeader(GMessageType type, uint32_t target,
                           uint8_t totalFrames, uint8_t frameIdx,
                           uint16_t payloadLen, uint8_t flags,
                           uint16_t sessionID, uint32_t seq,
                           uint8_t ttl = DEFAULT_TTL) {
    GPacket p;
    memset(&p, 0, sizeof(p));
    p.stx = STX_BYTE; p.version = G_VERSION; p.flags = flags;
    p.senderID = myID; p.targetID = target; p.sessionID = sessionID;
    p.msgType = (uint8_t)type; p.ttl = ttl;
    p.frameIdx = frameIdx; p.totalFrames = totalFrames;
    p.payloadLen = payloadLen; p.timestamp = nowMs() / 1000;
    p.seq = seq; p.etx = ETX_BYTE;
    return p;
}

// ============================================================================
// 13. OUTGOING
// ============================================================================
static OutgoingMessage* outgoingAlloc() {
    for (int i = 0; i < MAX_OUTGOING; i++)
        if (!outgoing[i].active) return &outgoing[i];
    return nullptr;
}

static void outgoingFree(OutgoingMessage* o) {
    if (o) memset(o, 0, sizeof(OutgoingMessage));
}

static bool sendFrameOfOutgoing(OutgoingMessage* o, uint8_t idx) {
    GPacket p = buildHeader((GMessageType)o->msgType, o->targetID,
                            o->totalFrames, idx, o->frameLens[idx],
                            o->flags, o->sessionID, g_sessionCounter++, DEFAULT_TTL);
    memcpy(p.payload, o->frameData[idx], o->frameLens[idx]);
    return enqueueTx(p);
}

static void outgoingStart(GMessageType type, uint32_t target,
                          const uint8_t* data, uint16_t len,
                          uint8_t extraFlags, bool needAck = true) {
    if (xSemaphoreTakeRecursive(stateMutex, portMAX_DELAY) != pdTRUE) return;
    OutgoingMessage* o = outgoingAlloc();
    if (!o) { Serial.println("[TX] outgoing full"); xSemaphoreGiveRecursive(stateMutex); return; }

    uint8_t totalFrames = (len == 0) ? 1 : ((len + FRAME_PAYLOAD - 1) / FRAME_PAYLOAD);
    if (totalFrames > MAX_FRAMES) {
        Serial.println("[TX] msg too long");
        xSemaphoreGiveRecursive(stateMutex); return;
    }

    memset(o, 0, sizeof(OutgoingMessage));
    o->active = true;
    o->targetID = target;
    o->sessionID = ++g_sessionCounter;
    o->msgType = (uint8_t)type;
    o->flags = extraFlags;
    if (target == BROADCAST_ID) o->flags |= FLAG_BROADCAST;
    else if (needAck) o->flags |= FLAG_ACK_REQ;
    o->totalFrames = totalFrames;
    o->retries = 0;

    for (uint8_t i = 0; i < totalFrames; i++) {
        uint16_t off = i * FRAME_PAYLOAD;
        uint16_t l = (len - off > FRAME_PAYLOAD) ? FRAME_PAYLOAD : (len - off);
        o->frameLens[i] = l;
        if (data) memcpy(o->frameData[i], data + off, l);
    }

    for (uint8_t i = 0; i < totalFrames; i++) sendFrameOfOutgoing(o, i);
    o->lastSend = nowMs();
    xSemaphoreGiveRecursive(stateMutex);
}

static void outgoingHandleAck(uint32_t sender, uint16_t session, uint8_t ackMask) {
    if (xSemaphoreTakeRecursive(stateMutex, portMAX_DELAY) != pdTRUE) return;
    for (int i = 0; i < MAX_OUTGOING; i++) {
        OutgoingMessage* o = &outgoing[i];
        if (!o->active || o->sessionID != session || o->targetID != sender) continue;
        o->ackedMask |= ackMask;
        uint8_t need = (1u << o->totalFrames) - 1;
        if ((o->ackedMask & need) == need) {
            Serial.printf("[TX] session %u done\n", session);
            outgoingFree(o);
        }
    }
    xSemaphoreGiveRecursive(stateMutex);
}

static void outgoingTick() {
    if (xSemaphoreTakeRecursive(stateMutex, portMAX_DELAY) != pdTRUE) return;
    unsigned long t = nowMs();
    for (int i = 0; i < MAX_OUTGOING; i++) {
        OutgoingMessage* o = &outgoing[i];
        if (!o->active) continue;
        if (t - o->lastSend < ACK_TIMEOUT_MS) continue;

        if (o->flags & FLAG_NO_RETRY) {
            outgoingFree(o);
            continue;
        }

        uint8_t need = (1u << o->totalFrames) - 1;
        uint8_t missing = need & ~o->ackedMask;
        if (o->retries >= ACK_MAX_RETRIES) {
            Serial.printf("[TX] session %u timeout\n", o->sessionID);
            outgoingFree(o); continue;
        }
        o->retries++;
        for (uint8_t f = 0; f < o->totalFrames; f++)
            if (missing & (1u << f)) sendFrameOfOutgoing(o, f);
        o->lastSend = t;
    }
    xSemaphoreGiveRecursive(stateMutex);
}

// ============================================================================
// 14. REASSEMBLY
// ============================================================================
static IncomingMessage* rxSlotFor(const GPacket& p, bool create) {
    for (int i = 0; i < RX_SLOT_COUNT; i++)
        if (rxSlots[i].active && rxSlots[i].senderID == p.senderID
            && rxSlots[i].sessionID == p.sessionID) return &rxSlots[i];
    if (!create) return nullptr;
    for (int i = 0; i < RX_SLOT_COUNT; i++) {
        if (!rxSlots[i].active) {
            memset(&rxSlots[i], 0, sizeof(IncomingMessage));
            rxSlots[i].active = true;
            rxSlots[i].senderID = p.senderID;
            rxSlots[i].sessionID = p.sessionID;
            rxSlots[i].totalFrames = (p.totalFrames > MAX_FRAMES) ? MAX_FRAMES : p.totalFrames;
            rxSlots[i].lastUpdate = nowMs();
            return &rxSlots[i];
        }
    }
    return nullptr;
}

static void rxSlotsSweep() {
    if (xSemaphoreTakeRecursive(stateMutex, portMAX_DELAY) != pdTRUE) return;
    unsigned long t = nowMs();
    for (int i = 0; i < RX_SLOT_COUNT; i++)
        if (rxSlots[i].active && (t - rxSlots[i].lastUpdate) > RX_SESSION_TIMEOUT)
            memset(&rxSlots[i], 0, sizeof(IncomingMessage));
    xSemaphoreGiveRecursive(stateMutex);
}

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

// Forward
static void processFinalMessage(GPacket& frame, uint8_t* data, uint16_t len);

static void handlePacketReassembly(GPacket& p) {
    IncomingMessage* slot = rxSlotFor(p, true);
    if (!slot) return;
    if (p.frameIdx < MAX_FRAMES) {
        uint8_t bit = 1u << p.frameIdx;
        if ((slot->frameMask & bit) == 0) {
            uint16_t off = p.frameIdx * FRAME_PAYLOAD;
            uint16_t l = p.payloadLen;
            if (l > FRAME_PAYLOAD) l = FRAME_PAYLOAD;
            if (off + l <= MAX_MESSAGE_BYTES) {
                memcpy(slot->data + off, p.payload, l);
                slot->frameMask |= bit;
                slot->totalPayloadLen += l;
                slot->lastUpdate = nowMs();
            }
        }
    }
    uint8_t need = (1u << slot->totalFrames) - 1;
    if ((slot->frameMask & need) == need) {
        processFinalMessage(p, slot->data, slot->totalPayloadLen);
        if (p.targetID == myID && (p.flags & FLAG_ACK_REQ))
            sendAckBitmap(p.senderID, p.sessionID, slot->frameMask);
        memset(slot, 0, sizeof(IncomingMessage));
    } else if (p.targetID == myID && (p.flags & FLAG_ACK_REQ)) {
        sendAckBitmap(p.senderID, p.sessionID, slot->frameMask);
    }
}

// ============================================================================
// 15. ONION ROUTING
// ----------------------------------------------------------------------------
// Layer (dopo decifratura): [next_hop:4][type:1][len:2][payload]
//   next_hop == 0  -> siamo destinatario finale
//   next_hop != 0  -> forward
// AAD layer = [version:1][orig_sender:4][this_hop:4]
// Multi-frame nativo via outgoingStart.
// ============================================================================

static void buildOnionAAD(uint32_t origSender, uint32_t thisHop,
                          uint8_t* aad, size_t& aadLen) {
    size_t i = 0;
    aad[i++] = G_VERSION;
    memcpy(aad + i, &origSender, 4); i += 4;
    memcpy(aad + i, &thisHop, 4);    i += 4;
    aadLen = i;
}

// Dispatch di un payload consegnato dall'onion (già autenticato dal layer GCM)
static void dispatchOnionDelivered(uint32_t origSender, uint8_t type,
                                   uint8_t* data, uint16_t len) {
    switch (type) {
        case MSG_HEARTBEAT:
            Serial.printf("[ONION] HB da %08X\n", origSender);
            break;
        case MSG_SOS_MEDIC:
            Serial.printf("[ONION] !!! SOS MEDICO da %08X !!!\n", origSender);
            updateUI("SOS", "SOS (onion)");
            break;
        case MSG_CHAT: {
            char buf[MAX_MESSAGE_BYTES + 1];
            uint16_t n = len > MAX_MESSAGE_BYTES ? MAX_MESSAGE_BYTES : len;
            memcpy(buf, data, n); buf[n] = '\0';
            Serial.printf("[ONION] Chat da %08X: %s\n", origSender, buf);
            updateUI("CHAT", buf);
            break;
        }
        default:
            Serial.printf("[ONION] tipo 0x%02X len=%u da %08X\n",
                          type, len, origSender);
            break;
    }
}

static void handleOnionLayer(uint32_t origSender, uint8_t* data, uint16_t len) {
    if (xSemaphoreTakeRecursive(stateMutex, portMAX_DELAY) != pdTRUE) return;

    PeerSession* ps = peerFind(origSender);
    if (!ps || !ps->established) {
        Serial.printf("[ONION] no session with %08X\n", origSender);
        xSemaphoreGiveRecursive(stateMutex); return;
    }

    uint16_t plainLen = 0;
    uint8_t aad[ONION_AAD_LEN]; size_t aadLen;
    buildOnionAAD(origSender, myID, aad, aadLen);
    if (!gcmDecryptAAD(ps->shared_secret, aad, aadLen,
                       data, len, s_onionPlain, plainLen)) {
        Serial.println("[ONION] decrypt fail");
        xSemaphoreGiveRecursive(stateMutex); return;
    }
    if (plainLen < ONION_LAYER_HDR) {
        xSemaphoreGiveRecursive(stateMutex); return;
    }

    uint32_t nextHop = ((uint32_t)s_onionPlain[0] << 24) | ((uint32_t)s_onionPlain[1] << 16)
                     | ((uint32_t)s_onionPlain[2] <<  8) |  (uint32_t)s_onionPlain[3];
    uint8_t  nextType = s_onionPlain[4];
    uint16_t nextLen  = ((uint16_t)s_onionPlain[5] << 8) | s_onionPlain[6];
    uint8_t* nextData = s_onionPlain + ONION_LAYER_HDR;

    if (nextLen > plainLen - ONION_LAYER_HDR) {
        Serial.println("[ONION] len mismatch");
        xSemaphoreGiveRecursive(stateMutex); return;
    }

    // Copia ciphertext interno (s_onionPlain verrà riusato)
    memmove(s_onionB, nextData, nextLen);

    if (nextHop == 0) {
        if (nextType == MSG_ONION || nextType == MSG_ACK) {
            Serial.printf("[ONION] inner type forbidden 0x%02X\n", nextType);
            xSemaphoreGiveRecursive(stateMutex); return;
        }
        dispatchOnionDelivered(origSender, nextType, s_onionB, nextLen);
        Serial.printf("[ONION] final delivered from %08X len=%u\n", origSender, nextLen);
    } else {
        if (nextType != MSG_ONION) {
            Serial.printf("[ONION] inner non-onion 0x%02X\n", nextType);
            xSemaphoreGiveRecursive(stateMutex); return;
        }
        outgoingStart(MSG_ONION, nextHop, s_onionB, nextLen,
                      FLAG_RELAY_OK | FLAG_NO_RETRY, false);
        Serial.printf("[ONION] relay -> %08X (%u B)\n", nextHop, nextLen);
    }

    xSemaphoreGiveRecursive(stateMutex);
}

static bool onionSend(GMessageType finalType, uint32_t target,
                      const uint8_t* data, uint16_t dataLen,
                      const uint32_t* hops, uint8_t nHops) {
    if (xSemaphoreTakeRecursive(stateMutex, portMAX_DELAY) != pdTRUE) return false;

    if (nHops > ONION_MAX_INTERMEDIATES) {
        Serial.println("[ONION] too many hops");
        xSemaphoreGiveRecursive(stateMutex); return false;
    }

    uint32_t path[ONION_MAX_PATH];
    for (uint8_t i = 0; i < nHops; i++) path[i] = hops[i];
    path[nHops] = target;
    uint8_t pathLen = nHops + 1;

    for (uint8_t i = 0; i < pathLen; i++) {
        if (path[i] == BROADCAST_ID) {
            Serial.println("[ONION] no broadcast");
            xSemaphoreGiveRecursive(stateMutex); return false;
        }
        PeerSession* ps = peerFind(path[i]);
        if (!ps || !ps->established) {
            Serial.printf("[ONION] no session with %08X\n", path[i]);
            xSemaphoreGiveRecursive(stateMutex); return false;
        }
    }

    uint16_t maxPayload = MAX_MESSAGE_BYTES - ONION_LAYER_OVERHEAD * pathLen;
    if (dataLen > maxPayload) {
        Serial.printf("[ONION] payload %u > %u\n", dataLen, maxPayload);
        xSemaphoreGiveRecursive(stateMutex); return false;
    }

    uint16_t bufLen = 0;
    uint8_t* cur = s_onionA;
    uint8_t* nxt = s_onionB;

    // Layer 0: per il target
    {
        uint16_t i = 0;
        nxt[i++] = 0; nxt[i++] = 0; nxt[i++] = 0; nxt[i++] = 0;
        nxt[i++] = (uint8_t)finalType;
        nxt[i++] = (dataLen >> 8) & 0xFF;
        nxt[i++] = dataLen & 0xFF;
        if (dataLen) { memcpy(nxt + i, data, dataLen); i += dataLen; }

        PeerSession* tps = peerFind(target);
        uint8_t aad[ONION_AAD_LEN]; size_t aadLen;
        buildOnionAAD(myID, target, aad, aadLen);
        uint16_t outLen;
        if (!gcmEncryptAAD(tps->shared_secret, aad, aadLen, nxt, i, cur, outLen)) {
            Serial.println("[ONION] enc layer0 fail");
            xSemaphoreGiveRecursive(stateMutex); return false;
        }
        bufLen = outLen;
    }

    // Layer intermedi (dal più interno al più esterno)
    for (int i = (int)nHops - 1; i >= 0; i--) {
        PeerSession* ps = peerFind(path[i]);
        uint32_t nextHop = path[i + 1];

        uint16_t j = 0;
        nxt[j++] = (nextHop >> 24) & 0xFF;
        nxt[j++] = (nextHop >> 16) & 0xFF;
        nxt[j++] = (nextHop >>  8) & 0xFF;
        nxt[j++] = (nextHop      ) & 0xFF;
        nxt[j++] = (uint8_t)MSG_ONION;
        nxt[j++] = (bufLen >> 8) & 0xFF;
        nxt[j++] = bufLen & 0xFF;
        memcpy(nxt + j, cur, bufLen); j += bufLen;

        uint8_t aad[ONION_AAD_LEN]; size_t aadLen;
        buildOnionAAD(myID, path[i], aad, aadLen);
        uint16_t outLen;
        if (!gcmEncryptAAD(ps->shared_secret, aad, aadLen, nxt, j, cur, outLen)) {
            Serial.printf("[ONION] enc layer %d fail\n", i);
            xSemaphoreGiveRecursive(stateMutex); return false;
        }
        bufLen = outLen;
    }

    uint32_t firstHop = (nHops > 0) ? path[0] : target;
    Serial.printf("[ONION] send pathLen=%u target=%08X len=%u\n", pathLen, target, bufLen);
    outgoingStart(MSG_ONION, firstHop, cur, bufLen,
                  FLAG_RELAY_OK | FLAG_NO_RETRY, false);

    xSemaphoreGiveRecursive(stateMutex);
    return true;
}

// ============================================================================
// 16. FINAL MESSAGE DISPATCH
// ============================================================================
static void handleKeyExchange(uint8_t type, const uint8_t* data, uint16_t len,
                              uint32_t remoteID) {
    if (type == MSG_KYBER_PUBKEY) {
        if (len != KYBER_PK_SIZE) { Serial.println("[KYBER] PK size"); return; }
        static uint8_t ct[KYBER_CT_SIZE];
        uint8_t shared[32];
        pqc_kyber512_encapsulate(ct, shared, data);
        PeerSession* ps = peerGetOrCreate(remoteID);
        memcpy(ps->shared_secret, shared, 32);
        ps->established = true; ps->tx_seq = 1; ps->last_rx_seq = 0;
        ps->last_activity = nowMs();
        outgoingStart(MSG_KYBER_CIPHERTEXT, remoteID, ct, KYBER_CT_SIZE, FLAG_RELAY_OK);
        Serial.printf("[KYBER] session %08X ready (responder)\n", remoteID);
        updateUI("SECURE", "Handshake OK");
    } else if (type == MSG_KYBER_CIPHERTEXT) {
        if (len != KYBER_CT_SIZE) { Serial.println("[KYBER] CT size"); return; }
        if (!g_kyber_ready) { Serial.println("[KYBER] no keypair"); return; }
        uint8_t shared[32];
        pqc_kyber512_decapsulate(shared, data, g_kyber_sk);
        PeerSession* ps = peerGetOrCreate(remoteID);
        memcpy(ps->shared_secret, shared, 32);
        ps->established = true; ps->tx_seq = 1; ps->last_rx_seq = 0;
        ps->last_activity = nowMs();
        Serial.printf("[KYBER] session %08X ready (initiator)\n", remoteID);
        updateUI("SECURE", "Handshake OK");
    }
}

// Dispatch di un messaggio finale con header completo (per AAD)
static void processFinalMessage(GPacket& frame, uint8_t* data, uint16_t len) {
    uint8_t  type   = frame.msgType;
    uint32_t sender = frame.senderID;

    if (type == MSG_ONION) {
        handleOnionLayer(sender, data, len);
        return;
    }
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
    if (type == MSG_NACK) { Serial.printf("[RX] NACK %08X\n", sender); return; }

    // Avviso plaintext unicast
    if (!(frame.flags & FLAG_ENCRYPTED) && (frame.flags & FLAG_PLAINTEXT_WARN)
        && frame.targetID != BROADCAST_ID) {
        Serial.printf("[RX] *** PLAINTEXT da %08X ***\n", sender);
        updateUI("PLAINTEXT!", "msg in chiaro");
    }

    const uint8_t* plain = data;
    uint16_t plainLen = len;
    static uint8_t tmp[MAX_MESSAGE_BYTES];

    if (frame.flags & FLAG_ENCRYPTED) {
        PeerSession* ps = peerFind(sender);
        if (!ps || !ps->established) {
            Serial.printf("[RX] enc from unknown %08X\n", sender);
            return;
        }
        uint16_t outLen = 0;
        if (!decryptGCM_AAD(ps->shared_secret, frame, data, len, tmp, outLen)) {
            Serial.println("[RX] GCM auth fail");
            return;
        }
        if (frame.seq <= ps->last_rx_seq) {
            Serial.println("[RX] replay respinto");
            return;
        }
        ps->last_rx_seq = frame.seq;
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
            memcpy(buf, plain, n); buf[n] = '\0';
            Serial.printf("[RX] Chat %08X: %s\n", sender, buf);
            updateUI("CHAT", buf);
            break;
        }
        default:
            Serial.printf("[RX] type 0x%02X len=%u\n", type, plainLen);
            break;
    }
}

// ============================================================================
// 17. RICEZIONE
// ============================================================================
static void handleReceivedPacket(GPacket& p, float rssi, float snr) {
    if (xSemaphoreTakeRecursive(stateMutex, portMAX_DELAY) != pdTRUE) return;
    if (dedupCheckAndInsert(p.senderID, p.sessionID, p.frameIdx)) {
        xSemaphoreGiveRecursive(stateMutex); return;
    }
    bool isForMe     = (p.targetID == myID);
    bool isBroadcast = (p.targetID == BROADCAST_ID);
    bool forMe       = isForMe || isBroadcast;
    bool relayable   = (p.flags & FLAG_RELAY_OK) && p.ttl > 0 && p.senderID != myID;

    if (!forMe) foreign_packets_count++;

    // Relay generico (non onion: gli onion sono gestiti dal layer logico)
    if (relayable && p.targetID != myID && p.msgType != MSG_ONION) {
        GPacket r = p; r.ttl--; enqueueTx(r);
    }
    if (forMe) {
        updateUI("RX", "Msg in arrivo", rssi, snr);
        handlePacketReassembly(p);
    }
    lastActivity = nowMs();
    xSemaphoreGiveRecursive(stateMutex);
}

// ============================================================================
// 18. SEND SECURE OR PLAIN
// ============================================================================
static void sendSecureOrPlain(GMessageType type, uint32_t target,
                              const uint8_t* data, uint16_t len) {
    PeerSession* ps = peerFind(target);
    bool doEnc = (ps && ps->established && target != BROADCAST_ID);

    if (doEnc && len + ENC_OVERHEAD <= FRAME_PAYLOAD) {
        uint8_t flags = FLAG_RELAY_OK | FLAG_ENCRYPTED;
        if (target != BROADCAST_ID) flags |= FLAG_ACK_REQ;
        uint16_t sessionID = ++g_sessionCounter;
        uint32_t seq = ps->tx_seq++;
        GPacket p = buildHeader(type, target, 1, 0, len + ENC_OVERHEAD, flags,
                                sessionID, seq, DEFAULT_TTL);
        uint8_t blob[FRAME_PAYLOAD];
        uint16_t blobLen = 0;
        if (encryptGCM_AAD(ps->shared_secret, p, data, len, blob, blobLen)) {
            memcpy(p.payload, blob, blobLen);
            p.payloadLen = blobLen;
            enqueueTx(p);
            return;
        }
        Serial.println("[TX] encrypt fail -> plaintext");
    } else if (doEnc) {
        Serial.println("[TX] payload too big -> plaintext");
    }

    uint8_t flags = FLAG_RELAY_OK;
    if (target == BROADCAST_ID) {
        flags |= FLAG_BROADCAST;
    } else {
        flags |= FLAG_PLAINTEXT_WARN;
        g_plaintextWarnUntil = nowMs() + 3000;
        Serial.printf("[TX] *** PLAINTEXT a %08X ***\n", target);
    }
    outgoingStart(type, target, data, len, flags);
}

// ============================================================================
// 19. HEARTBEAT
// ============================================================================
static unsigned long lastHeartbeat = 0;
static void heartbeatTick() {
    if (xSemaphoreTakeRecursive(stateMutex, portMAX_DELAY) != pdTRUE) return;
    if (nowMs() - lastHeartbeat < 60000) {
        xSemaphoreGiveRecursive(stateMutex); return;
    }
    lastHeartbeat = nowMs();
    outgoingStart(MSG_HEARTBEAT, BROADCAST_ID, nullptr, 0, FLAG_RELAY_OK);
    xSemaphoreGiveRecursive(stateMutex);
}

// ============================================================================
// 20. WEB UI
// ============================================================================
static bool webAuth(AsyncWebServerRequest* r) {
    if (!r->authenticate(WEB_USER, WEB_PASS)) {
        r->requestAuthentication();
        return false;
    }
    return true;
}

static uint8_t parseHops(const String& s, uint32_t* out, uint8_t maxN) {
    uint8_t n = 0;
    int start = 0;
    while (start < (int)s.length() && n < maxN) {
        int comma = s.indexOf(',', start);
        String tok = (comma < 0) ? s.substring(start) : s.substring(start, comma);
        tok.trim();
        if (tok.length() > 0) {
            char* endp = nullptr;
            uint32_t id = strtoul(tok.c_str(), &endp, 16);
            if (endp && *endp == '\0') out[n++] = id;
        }
        if (comma < 0) break;
        start = comma + 1;
    }
    return n;
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
        h += "<title>Qorlyn</title><style>";
        h += "body{font-family:sans-serif;background:#181818;color:#eee;text-align:center;padding:12px}";
        h += "button{padding:14px 20px;margin:6px;border:none;border-radius:6px;font-size:16px;cursor:pointer}";
        h += ".sos{background:#d32f2f;color:#fff}.msg{background:#388e3c;color:#fff}";
        h += ".key{background:#fbc02d;color:#000}.onion{background:#5e35b1;color:#fff}";
        h += "input{padding:10px;width:85%;margin:6px;border-radius:6px;border:none}";
        h += ".hint{font-size:12px;color:#888;margin-top:4px}";
        h += "</style></head><body>";
        char idbuf[16]; sprintf(idbuf, "%08X", myID);
        h += "<h2>Qorlyn</h2><div>ID: "; h += idbuf; h += "</div>";
        h += "<input id='t' placeholder='Target ID (hex)' value='FFFFFFFF'>";
        h += "<input id='hops' placeholder='Onion hops (hex, comma sep.)' value=''>";
        h += "<div class='hint'>es. <code>DEADBEEF,12345678</code> = 2 hop intermedi</div>";
        h += "<input id='m' placeholder='Messaggio'>";
        h += "<br><button class='msg' onclick=\"s('CHAT')\">CHAT</button>";
        h += "<button class='sos' onclick=\"s('SOS')\">SOS MEDICO</button>";
        h += "<button class='key' onclick=\"s('KEY')\">HANDSHAKE KYBER</button>";
        h += "<button class='onion' onclick=\"s('ONION')\">ONION</button>";
        h += "<script>";
        h += "function s(t){";
        h += "var tg=document.getElementById('t').value;";
        h += "var hp=document.getElementById('hops').value;";
        h += "var msg=document.getElementById('m').value;";
        h += "send(false);";
        h += "function send(force){";
        h += "fetch('/send',{method:'POST',";
        h += "headers:{'Content-Type':'application/x-www-form-urlencoded'},";
        h += "body:'type='+encodeURIComponent(t)+";
        h += "'&target='+encodeURIComponent(tg)+";
        h += "'&hops='+encodeURIComponent(hp)+";
        h += "'&msg='+encodeURIComponent(msg)+";
        h += "'&force='+(force?1:0)}).then(function(r){";
        h += "if(r.status===409){";
        h += "if(confirm('Il destinatario non ha una sessione sicura.\\nInviare comunque IN CHIARO?')) send(true);";
        h += "}else if(r.status!==200){ r.text().then(function(x){alert('Errore: '+x);}); }";
        h += "});}";
        h += "}";
        h += "</script>";
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
        String type    = req->getParam("type", true)->value();
        String tstr    = req->getParam("target", true)->value();
        String hopsStr = req->hasParam("hops", true) ? req->getParam("hops", true)->value() : "";
        String msg     = req->hasParam("msg", true) ? req->getParam("msg", true)->value() : "";
        bool force     = req->hasParam("force", true) && req->getParam("force", true)->value() == "1";

        char* endp = nullptr;
        uint32_t target = strtoul(tstr.c_str(), &endp, 16);
        if (!endp || *endp != '\0' || tstr.length() == 0) {
            req->send(400, "text/plain", "target non valido");
            return;
        }

        if (xSemaphoreTakeRecursive(stateMutex, portMAX_DELAY) != pdTRUE) {
            req->send(500, "text/plain", "mutex error");
            return;
        }

        if (type == "ONION") {
            uint32_t hops[ONION_MAX_INTERMEDIATES];
            uint8_t nHops = parseHops(hopsStr, hops, ONION_MAX_INTERMEDIATES);
            if (msg.length() > MAX_MESSAGE_BYTES) {
                xSemaphoreGiveRecursive(stateMutex);
                req->send(413, "text/plain", "msg troppo lungo");
                return;
            }
            bool ok = onionSend(MSG_CHAT, target,
                                (const uint8_t*)msg.c_str(), msg.length(),
                                hops, nHops);
            xSemaphoreGiveRecursive(stateMutex);
            if (!ok) { req->send(400, "text/plain", "onion send failed"); return; }
            req->send(200, "text/plain", "ok");
            return;
        }

        // Check plaintext fallback per SOS/CHAT unicast
        if ((type == "SOS" || type == "CHAT") && target != BROADCAST_ID) {
            PeerSession* ps = peerFind(target);
            bool willEncrypt = (ps && ps->established);
            if (type == "CHAT" && msg.length() + ENC_OVERHEAD > FRAME_PAYLOAD) {
                willEncrypt = false;
            }
            if (!willEncrypt && !force) {
                xSemaphoreGiveRecursive(stateMutex);
                req->send(409, "text/plain", "PLAINTEXT_FALLBACK");
                return;
            }
        }

        if (type == "SOS") {
            sendSecureOrPlain(MSG_SOS_MEDIC, target, nullptr, 0);
        } else if (type == "CHAT") {
            if (msg.length() > MAX_MESSAGE_BYTES) {
                xSemaphoreGiveRecursive(stateMutex);
                req->send(413, "text/plain", "msg troppo lungo");
                return;
            }
            sendSecureOrPlain(MSG_CHAT, target, (const uint8_t*)msg.c_str(), msg.length());
        } else if (type == "KEY") {
            if (target == BROADCAST_ID) {
                xSemaphoreGiveRecursive(stateMutex);
                req->send(400, "text/plain", "no broadcast handshake");
                return;
            }
            kyberEnsureKeypair();
            outgoingStart(MSG_KYBER_PUBKEY, target, g_kyber_pk, KYBER_PK_SIZE, FLAG_RELAY_OK);
        } else {
            xSemaphoreGiveRecursive(stateMutex);
            req->send(400, "text/plain", "tipo sconosciuto");
            return;
        }

        xSemaphoreGiveRecursive(stateMutex);
        req->send(200, "text/plain", "ok");
    });

    server.begin();
}

// ============================================================================
// 21. SERIAL
// ============================================================================
static void serialTick() {
    if (xSemaphoreTakeRecursive(stateMutex, portMAX_DELAY) != pdTRUE) return;
    while (Serial.available()) {
        int c = Serial.read();
        switch (c) {
            case 'h': {
                const char* m = "Hello world";
                sendSecureOrPlain(MSG_CHAT, BROADCAST_ID, (const uint8_t*)m, strlen(m));
                break;
            }
            case 'k':
                kyberEnsureKeypair();
                Serial.println("[KYBER] PK ready");
                break;
            case 'i':
                Serial.printf("FEC=%u INT=%u foreign=%lu crc=%lu\n",
                              DynamicFEC(interference_level), interference_level,
                              (unsigned long)foreign_packets_count,
                              (unsigned long)crc_error_count);
                break;
            case 'p':
                for (int i = 0; i < MAX_PEERS; i++)
                    if (peers[i].inUse)
                        Serial.printf("peer %08X est=%d last_rx=%u\n",
                                      peers[i].peerID, peers[i].established,
                                      peers[i].last_rx_seq);
                break;
        }
    }
    xSemaphoreGiveRecursive(stateMutex);
}

// ============================================================================
// 22. SLEEP
// ============================================================================
static void enterSleep() {
    Serial.println("[SLEEP] deep sleep");
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
// 23. RADIO SETUP
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
// 24. OLED SETUP
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
    oled.println("Qorlyn boot...");
    oled.display();
}

// ============================================================================
// 25. SETUP / LOOP
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("\n=== Qorlyn ===");
    Serial.printf("GPacket=%u Incoming=%u Outgoing=%u\n",
                  (unsigned)sizeof(GPacket),
                  (unsigned)sizeof(IncomingMessage),
                  (unsigned)sizeof(OutgoingMessage));
    Serial.printf("ONION max payload: %u B (path max %u)\n",
                  (unsigned)ONION_MAX_PAYLOAD, (unsigned)ONION_MAX_PATH);

    myID = g_myID ? g_myID : generateDeviceID();
    g_myID = myID;
    Serial.printf("myID=%08X\n", myID);

    stateMutex = xSemaphoreCreateRecursiveMutex();
    txQueue    = xQueueCreate(16, sizeof(TxRequest));

    memset(peers, 0, sizeof(peers));
    memset(rxSlots, 0, sizeof(rxSlots));
    memset(outgoing, 0, sizeof(outgoing));

    setupOLED();
    printWakeReason();

    if (!setupRadio()) updateUI("ERROR", "radio fail");

    xTaskCreatePinnedToCore(radioTask, "radio", 8192, nullptr, 2, &radioTaskHandle, 1);

    if (!g_kyber_ready) kyberEnsureKeypair();
    else Serial.println("[KYBER] keypair da RTC");

    setupWeb();

    lastActivity = nowMs();
    updateUI("READY", "online");
}

void loop() {
    if (nowMs() - lastActivity > 10UL * 60UL * 1000UL)
        enterSleep();

    outgoingTick();
    rxSlotsSweep();
    peersSweep();
    heartbeatTick();
    serialTick();

    if (nowMs() < g_plaintextWarnUntil) updateUI("PLAINTEXT!", "msg in chiaro");
    else updateUI("READY", "online");

    delay(20);
}

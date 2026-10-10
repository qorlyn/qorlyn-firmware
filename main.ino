/* ============================================================================
 * Qorlyn Firmware - V.0.0.1 (Alpha)
 *
 * Copyright (C) 2026 Emanuele Ferraro (github.com/emanueleferraro)
 * Copyright (C) 2026 Qorlyn (github.com/qorlyn)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * ----------------------------------------------------------------------------
 * ML-KEM (Kyber-512) + AES-256-GCM + Onion routing (X25519 effimero per-hop)
 * + Multi-protocol transport (LoRa / WiFi STA+AP / BLE peripheral+central)
 * ----------------------------------------------------------------------------
 * Connect with us:
 *   🌐 Website:  https://qorlyn.org
 *   📧 Email:    qorlyn@qorlyn.org
 *   💬 Forum:    https://forum.qorlyn.org
 *    @ Discord:  qorlyn.org/discord
 *    @ Telegram: qorlyn.org/telegram
 *    @ GitHub:   qorlyn.org/github
 *    Donate:     qorlyn.org/donate
 NOTE: CODE NOT TESTED.
 * ==========================================================================*/

#define MBEDTLS_ALLOW_PRIVATE_ACCESS
#include <Arduino.h>
#include <esp_mac.h>
#include <esp_sleep.h>
#include <esp_attr.h>
#include "mbedtls/gcm.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/ecp.h"
#include <RadioLib.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEClient.h>
#include <BLEUtils.h>
#include <BLE2902.h>
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
#define OLED_PWR 36
//#define OLED_PWR 32
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
#define G_VERSION            0x04
#define STX_BYTE             0x02
#define ETX_BYTE             0x03

#define DEFAULT_TTL          3
#define MAX_FRAMES           8
#define FRAME_PAYLOAD        128
#define MAX_MESSAGE_BYTES    (MAX_FRAMES * FRAME_PAYLOAD)
#define BROADCAST_ID         0xFFFFFFFFu

#define CACHE_SIZE           32
#define MAX_PEERS            12
#define MAX_OUTGOING         6
#define ACK_TIMEOUT_MS       3000
#define ACK_MAX_RETRIES      3
#define RX_SLOT_COUNT        4
#define RX_SESSION_TIMEOUT   120000UL
#define HEARTBEAT_INTERVAL_MS 30000UL
#define LASTSEEN_RESET_MS    60000UL
#define STA_RETRY_INTERVAL_MS 60000UL

#define KYBER_PK_SIZE        800
#define KYBER_SK_SIZE        1632
#define KYBER_CT_SIZE        768

// X25519
#define X25519_KEY_SIZE      32

// Onion
#define ONION_MAX_HOPS       6
#define ONION_LAYER_HDR      8
#define ONION_EPH_PUB        32
#define ONION_LAYER_OVERHEAD (ONION_EPH_PUB + 12 + 16 + ONION_LAYER_HDR)
#define ONION_TOKEN_LEN      4
#define ONION_INNER_HDR      8
#define ONION_MAX_PAYLOAD    (MAX_MESSAGE_BYTES - ONION_LAYER_OVERHEAD * ONION_MAX_HOPS - ONION_INNER_HDR)
#define ONION_TRACKER_SIZE   4
#define ONION_TRACKER_TTL    60000UL

// WiFi
#define WIFI_SSID_PREFIX     "QORLYN-"
#define WIFI_PASSWORD        "gmesh2026"
#define WEB_USER             "admin"
#define WEB_PASS             "gmesh2026"
#define UDP_DATA_PORT        4200
#define WIFI_STA_TIMEOUT_MS  8000

// BLE
#define BLE_DEVICE_PREFIX    "QORLYN"
#define BLE_SVC_UUID         "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define BLE_RX_CHAR_UUID     "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define BLE_TX_CHAR_UUID     "6e400003-b5a3-f393-e0a9-e50e24dcca9e"
#define BLE_MTU_PAYLOAD      480
#define BLE_SCAN_INTERVAL_MS 15000
#define BLE_MAX_CENTRALS     2

// Peer
#define PENDING_MAX          4
#define PENDING_TIMEOUT      300000UL
#define HYBRID_AUTO_LIMIT    3
#define BLOCKLIST_MAX        8
#define WHITELIST_MAX        8
#define REQUEST_MIN_INTERVAL 2000UL

// RTC magic per distinguere cold boot da warm reset
#define RTC_MAGIC            0x514F524Cu

// ============================================================================
// 3. TIPI
// ============================================================================
enum GMessageType : uint8_t {
    MSG_HEARTBEAT        = 0x01,
    MSG_SOS_MEDIC        = 0x03,
    MSG_ACK              = 0x08,
    MSG_NACK             = 0x1B,
    MSG_CHAT             = 0x1C,
    MSG_KYBER_PUBKEY     = 0x20,
    MSG_KYBER_CIPHERTEXT = 0x21,
    MSG_ONION            = 0x22,
    MSG_ONION_ACK        = 0x23,
    MSG_KYBER_REJECT     = 0x24,
    MSG_X25519_PUBKEY    = 0x25
};

enum PacketFlags : uint8_t {
    FLAG_ENCRYPTED      = 0x01,
    FLAG_ACK_REQ        = 0x02,
    FLAG_RELAY_OK       = 0x04,
    FLAG_BROADCAST      = 0x08,
    FLAG_PLAINTEXT_WARN = 0x10,
    FLAG_NO_RETRY       = 0x20,
    FLAG_X25519_ENC     = 0x40
};

enum OnionFlags : uint8_t {
    ONION_FLAG_ACK_REQ  = 0x01
};

enum ProtoType : uint8_t {
    PROTO_LORA = 0,
    PROTO_WIFI = 1,
    PROTO_BT   = 2
};

enum PeerState : uint8_t {
    PEER_NONE        = 0,
    PEER_PENDING_IN  = 1,
    PEER_PENDING_OUT = 2,
    PEER_ESTABLISHED = 3
};

enum AcceptPolicy : uint8_t {
    POLICY_AUTO      = 0,
    POLICY_MANUAL    = 1,
    POLICY_WHITELIST = 2,
    POLICY_HYBRID    = 3
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
    uint8_t  originProto;
    uint8_t  payload[FRAME_PAYLOAD];
    uint8_t  etx;
};

struct PeerSession {
    PeerState state;
    uint32_t  peerID;
    uint8_t   shared_secret[32];
    uint8_t   x25519_pk[X25519_KEY_SIZE];
    bool      has_x25519_pk;
    uint32_t  last_rx_seq;
    uint32_t  tx_seq;
    unsigned long last_activity;
    unsigned long last_request_time;
    uint8_t   lastSeenProto;
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

struct OnionTracker {
    bool     active;
    uint32_t token;
    uint32_t targetID;
    unsigned long sentAt;
};

struct PendingPeer {
    bool     active;
    uint32_t peerID;
    uint8_t  pk[KYBER_PK_SIZE];
    unsigned long since;
};

// ============================================================================
// 5. GLOBALI
// ============================================================================
RTC_DATA_ATTR uint32_t g_sessionCounter = 0;
RTC_DATA_ATTR uint32_t g_myID           = 0;
RTC_DATA_ATTR uint8_t  g_kyber_pk[KYBER_PK_SIZE];
RTC_DATA_ATTR uint8_t  g_kyber_sk[KYBER_SK_SIZE];
RTC_DATA_ATTR bool     g_kyber_ready    = false;
RTC_DATA_ATTR uint8_t  g_x25519_sk[X25519_KEY_SIZE];
RTC_DATA_ATTR uint8_t  g_x25519_pk[X25519_KEY_SIZE];
RTC_DATA_ATTR bool     g_x25519_ready   = false;

RTC_DATA_ATTR PeerSession peers[MAX_PEERS];
RTC_DATA_ATTR uint8_t     g_policy           = POLICY_MANUAL;
RTC_DATA_ATTR uint32_t    g_blocklist[BLOCKLIST_MAX];
RTC_DATA_ATTR uint8_t     g_blocklist_count  = 0;
RTC_DATA_ATTR uint32_t    g_whitelist[WHITELIST_MAX];
RTC_DATA_ATTR uint8_t     g_whitelist_count  = 0;
RTC_DATA_ATTR uint32_t    g_rtc_magic        = 0;

uint32_t myID = 0;
SemaphoreHandle_t stateMutex = nullptr;
QueueHandle_t     txQueue    = nullptr;
QueueHandle_t     bleRxQueue = nullptr;
TaskHandle_t      radioTaskHandle = nullptr;
TaskHandle_t      bleTaskHandle   = nullptr;
TaskHandle_t      wifiTaskHandle  = nullptr;

Adafruit_SSD1306  oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
AsyncWebServer    server(80);
WiFiUDP           g_udp;

PendingPeer      g_pending[PENDING_MAX];
IncomingMessage  rxSlots[RX_SLOT_COUNT];
OutgoingMessage  outgoing[MAX_OUTGOING];
OnionTracker     g_onionTrackers[ONION_TRACKER_SIZE];

uint32_t dedupCache[CACHE_SIZE] = {0};
uint8_t  dedupIdx = 0;

volatile uint32_t foreign_packets_count = 0;
volatile uint32_t crc_error_count       = 0;
volatile uint8_t  interference_level    = 1;

unsigned long lastActivity = 0;
unsigned long g_plaintextWarnUntil = 0;

bool g_loraReady = false;
bool g_wifiReady = false;
bool g_btReady   = false;
volatile bool g_bleCentralConnected = false;
volatile bool g_bleScanEnabled = true;
uint8_t g_lastRxProto = PROTO_LORA;

BLEServer*         g_bleServer  = nullptr;
BLECharacteristic* g_bleTxChar  = nullptr;
BLECharacteristic* g_bleRxChar  = nullptr;

struct BleCentralLink {
    bool           active;
    BLEClient*     client;
    BLERemoteCharacteristic* txChar;
    BLERemoteCharacteristic* rxChar;
    uint32_t       peerID;
    unsigned long  lastSeen;
};
BleCentralLink g_bleCentrals[BLE_MAX_CENTRALS];

static char g_bleName[24];

#define ONION_BUF (MAX_MESSAGE_BYTES + 128)
static uint8_t s_onionA[ONION_BUF];
static uint8_t s_onionB[ONION_BUF];
static uint8_t s_onionPlain[ONION_BUF];

// ============================================================================
// 6. UTILITY
// ============================================================================
static inline uint32_t nowMs() { return millis(); }

static uint8_t DynamicFEC(uint8_t lvl) {
    if (lvl <= 2) return 5;
    if (lvl <= 5) return 6;
    if (lvl <= 8) return 7;
    return 8;
}

static const char* protoName(uint8_t p) {
    switch (p) {
        case PROTO_LORA: return "LoRa";
        case PROTO_WIFI: return "WiFi";
        case PROTO_BT:   return "BT";
        default:         return "?";
    }
}

static const char* peerStateName(PeerState s) {
    switch (s) {
        case PEER_NONE:        return "none";
        case PEER_PENDING_IN:  return "pending-in";
        case PEER_PENDING_OUT: return "pending-out";
        case PEER_ESTABLISHED: return "established";
        default:               return "?";
    }
}

static const char* policyName(uint8_t p) {
    switch (p) {
        case POLICY_AUTO:      return "auto";
        case POLICY_MANUAL:    return "manual";
        case POLICY_WHITELIST: return "whitelist";
        case POLICY_HYBRID:    return "hybrid";
        default:               return "?";
    }
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
    oled.print("Pol: ");
    oled.print(policyName(g_policy));
    int nEst = 0, nPend = 0;
    for (int i = 0; i < MAX_PEERS; i++) {
        if (peers[i].state == PEER_ESTABLISHED) nEst++;
        else if (peers[i].state == PEER_PENDING_IN || peers[i].state == PEER_PENDING_OUT) nPend++;
    }
    oled.setCursor(0, 22);
    oled.printf("Est:%d Pend:%d", nEst, nPend);
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
// 8. X25519 (ECP-level API)
// ============================================================================
static int x25519_rng(void* ctx, unsigned char* buf, size_t len) {
    (void)ctx;
    esp_fill_random(buf, len);
    return 0;
}

static bool x25519GenKeypair(uint8_t* sk, uint8_t* pk) {
    mbedtls_ecp_group grp;
    mbedtls_ecp_group_init(&grp);
    if (mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_CURVE25519) != 0) {
        mbedtls_ecp_group_free(&grp);
        return false;
    }
    mbedtls_mpi d;
    mbedtls_mpi_init(&d);
    if (mbedtls_ecp_gen_privkey(&grp, &d, x25519_rng, NULL) != 0) {
        mbedtls_mpi_free(&d);
        mbedtls_ecp_group_free(&grp);
        return false;
    }
    mbedtls_ecp_point Q;
    mbedtls_ecp_point_init(&Q);
    if (mbedtls_ecp_mul(&grp, &Q, &d, &grp.G, x25519_rng, NULL) != 0) {
        mbedtls_ecp_point_free(&Q);
        mbedtls_mpi_free(&d);
        mbedtls_ecp_group_free(&grp);
        return false;
    }
    if (mbedtls_mpi_write_binary(&d, sk, X25519_KEY_SIZE) != 0) {
        mbedtls_ecp_point_free(&Q);
        mbedtls_mpi_free(&d);
        mbedtls_ecp_group_free(&grp);
        return false;
    }
    if (mbedtls_mpi_write_binary(&Q.X, pk, X25519_KEY_SIZE) != 0) {
        mbedtls_ecp_point_free(&Q);
        mbedtls_mpi_free(&d);
        mbedtls_ecp_group_free(&grp);
        return false;
    }
    mbedtls_ecp_point_free(&Q);
    mbedtls_mpi_free(&d);
    mbedtls_ecp_group_free(&grp);
    return true;
}

static bool x25519Shared(const uint8_t* my_sk, const uint8_t* peer_pk, uint8_t* out) {
    mbedtls_ecp_group grp;
    mbedtls_ecp_group_init(&grp);
    if (mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_CURVE25519) != 0) {
        mbedtls_ecp_group_free(&grp);
        return false;
    }
    mbedtls_mpi d;
    mbedtls_mpi_init(&d);
    if (mbedtls_mpi_read_binary(&d, my_sk, X25519_KEY_SIZE) != 0) {
        mbedtls_mpi_free(&d);
        mbedtls_ecp_group_free(&grp);
        return false;
    }
    mbedtls_ecp_point Q;
    mbedtls_ecp_point_init(&Q);
    if (mbedtls_mpi_read_binary(&Q.X, peer_pk, X25519_KEY_SIZE) != 0) {
        mbedtls_ecp_point_free(&Q);
        mbedtls_mpi_free(&d);
        mbedtls_ecp_group_free(&grp);
        return false;
    }
    mbedtls_mpi_lset(&Q.Y, 1);
    mbedtls_mpi_lset(&Q.Z, 1);
    mbedtls_ecp_point S;
    mbedtls_ecp_point_init(&S);
    if (mbedtls_ecp_mul(&grp, &S, &d, &Q, x25519_rng, NULL) != 0) {
        mbedtls_ecp_point_free(&S);
        mbedtls_ecp_point_free(&Q);
        mbedtls_mpi_free(&d);
        mbedtls_ecp_group_free(&grp);
        return false;
    }
    if (mbedtls_mpi_write_binary(&S.X, out, 32) != 0) {
        mbedtls_ecp_point_free(&S);
        mbedtls_ecp_point_free(&Q);
        mbedtls_mpi_free(&d);
        mbedtls_ecp_group_free(&grp);
        return false;
    }
    mbedtls_ecp_point_free(&S);
    mbedtls_ecp_point_free(&Q);
    mbedtls_mpi_free(&d);
    mbedtls_ecp_group_free(&grp);
    return true;
}

static void x25519EnsureKeypair() {
    if (g_x25519_ready) return;
    Serial.println("[X25519] keypair...");
    if (!x25519GenKeypair(g_x25519_sk, g_x25519_pk)) {
        Serial.println("[X25519] FAIL");
        return;
    }
    g_x25519_ready = true;
    Serial.println("[X25519] ready");
}

static bool x25519Encrypt(const uint8_t* shared_key,
                          const uint8_t* aad, size_t aadLen,
                          const uint8_t* plaintext, uint16_t len,
                          uint8_t* out, uint16_t& outLen) {
    if (len + 28 > ONION_BUF) return false;
    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);
    if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, shared_key, 256) != 0) {
        mbedtls_gcm_free(&gcm); return false;
    }
    uint8_t iv[12];
    for (int i = 0; i < 12; i++) iv[i] = (uint8_t)esp_random();
    memcpy(out, iv, 12);
    uint8_t tag[16];
    int r = mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, len,
                                      iv, 12, aad, aadLen,
                                      plaintext, out + 28, 16, tag);
    memcpy(out + 12, tag, 16);
    mbedtls_gcm_free(&gcm);
    if (r != 0) return false;
    outLen = len + 28;
    return true;
}

static bool x25519Decrypt(const uint8_t* shared_key,
                          const uint8_t* aad, size_t aadLen,
                          const uint8_t* in, uint16_t inLen,
                          uint8_t* out, uint16_t& outLen) {
    if (inLen < 28) return false;
    uint16_t clen = inLen - 28;
    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);
    if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, shared_key, 256) != 0) {
        mbedtls_gcm_free(&gcm); return false;
    }
    uint8_t iv[12], tag[16];
    memcpy(iv, in, 12);
    memcpy(tag, in + 12, 16);
    int r = mbedtls_gcm_auth_decrypt(&gcm, clen, iv, 12, aad, aadLen,
                                     tag, 16, in + 28, out);
    mbedtls_gcm_free(&gcm);
    if (r != 0) return false;
    outLen = clen;
    return true;
}

// ============================================================================
// 9. CRITTOGRAFIA (Kyber sessions)
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

// ============================================================================
// 10. KYBER
// ============================================================================
static void kyberEnsureKeypair() {
    if (g_kyber_ready) return;
    Serial.println("[KYBER] keypair...");
    pqc_kyber512_keypair(g_kyber_pk, g_kyber_sk);
    g_kyber_ready = true;
    Serial.println("[KYBER] ready");
}

// ============================================================================
// 11. PEER
// ============================================================================
static PeerSession* peerFind(uint32_t id) {
    for (int i = 0; i < MAX_PEERS; i++)
        if (peers[i].state != PEER_NONE && peers[i].peerID == id) return &peers[i];
    return nullptr;
}

static PeerSession* peerAlloc() {
    for (int i = 0; i < MAX_PEERS; i++)
        if (peers[i].state == PEER_NONE) return &peers[i];
    int best = -1;
    unsigned long bestT = ULONG_MAX;
    for (int i = 0; i < MAX_PEERS; i++) {
        if (peers[i].state == PEER_ESTABLISHED) continue;
        if (peers[i].last_activity < bestT) { bestT = peers[i].last_activity; best = i; }
    }
    if (best >= 0) return &peers[best];
    for (int i = 0; i < MAX_PEERS; i++)
        if (peers[i].last_activity < bestT) { bestT = peers[i].last_activity; best = i; }
    return best >= 0 ? &peers[best] : nullptr;
}

static PeerSession* peerGetOrCreate(uint32_t id) {
    PeerSession* p = peerFind(id);
    if (p) return p;
    PeerSession* slot = peerAlloc();
    if (!slot) return nullptr;
    memset(slot, 0, sizeof(PeerSession));
    slot->peerID = id;
    slot->last_activity = nowMs();
    slot->lastSeenProto = PROTO_LORA;
    return slot;
}

static void peerForget(uint32_t id) {
    PeerSession* p = peerFind(id);
    if (p) memset(p, 0, sizeof(PeerSession));
    PendingPeer* pp = nullptr;
    for (int i = 0; i < PENDING_MAX; i++)
        if (g_pending[i].active && g_pending[i].peerID == id) { pp = &g_pending[i]; break; }
    if (pp) memset(pp, 0, sizeof(PendingPeer));
}

static int countEstablished() {
    int n = 0;
    for (int i = 0; i < MAX_PEERS; i++)
        if (peers[i].state == PEER_ESTABLISHED) n++;
    return n;
}

static bool isBlocked(uint32_t id) {
    for (uint8_t i = 0; i < g_blocklist_count; i++)
        if (g_blocklist[i] == id) return true;
    return false;
}

static bool addBlocklist(uint32_t id) {
    if (isBlocked(id)) return true;
    if (g_blocklist_count >= BLOCKLIST_MAX) return false;
    g_blocklist[g_blocklist_count++] = id;
    return true;
}

static bool removeBlocklist(uint32_t id) {
    for (uint8_t i = 0; i < g_blocklist_count; i++) {
        if (g_blocklist[i] == id) {
            g_blocklist[i] = g_blocklist[g_blocklist_count - 1];
            g_blocklist_count--;
            return true;
        }
    }
    return false;
}

static bool isWhitelisted(uint32_t id) {
    for (uint8_t i = 0; i < g_whitelist_count; i++)
        if (g_whitelist[i] == id) return true;
    return false;
}

static bool addWhitelist(uint32_t id) {
    if (isWhitelisted(id)) return true;
    if (g_whitelist_count >= WHITELIST_MAX) return false;
    g_whitelist[g_whitelist_count++] = id;
    return true;
}

static bool removeWhitelist(uint32_t id) {
    for (uint8_t i = 0; i < g_whitelist_count; i++) {
        if (g_whitelist[i] == id) {
            g_whitelist[i] = g_whitelist[g_whitelist_count - 1];
            g_whitelist_count--;
            return true;
        }
    }
    return false;
}

static PendingPeer* pendingFind(uint32_t id) {
    for (int i = 0; i < PENDING_MAX; i++)
        if (g_pending[i].active && g_pending[i].peerID == id) return &g_pending[i];
    return nullptr;
}

static PendingPeer* pendingAlloc() {
    for (int i = 0; i < PENDING_MAX; i++)
        if (!g_pending[i].active) return &g_pending[i];
    return nullptr;
}

static void pendingFree(uint32_t id) {
    PendingPeer* p = pendingFind(id);
    if (p) memset(p, 0, sizeof(PendingPeer));
}

static void peersSweep() {
    if (xSemaphoreTakeRecursive(stateMutex, portMAX_DELAY) != pdTRUE) return;
    unsigned long t = nowMs();
    for (int i = 0; i < MAX_PEERS; i++) {
        if (peers[i].state == PEER_NONE) continue;

        // Reset lastSeenProto se inattivo da troppo (peer potrebbe essere su altro proto)
        if ((t - peers[i].last_activity) > LASTSEEN_RESET_MS
            && peers[i].lastSeenProto != PROTO_LORA) {
            peers[i].lastSeenProto = PROTO_LORA;
        }

        if (peers[i].state == PEER_ESTABLISHED && (t - peers[i].last_activity > 30UL * 60UL * 1000UL)) {
            memset(&peers[i], 0, sizeof(PeerSession));
        }
        if ((peers[i].state == PEER_PENDING_IN || peers[i].state == PEER_PENDING_OUT)
            && (t - peers[i].last_activity > PENDING_TIMEOUT)) {
            pendingFree(peers[i].peerID);
            memset(&peers[i], 0, sizeof(PeerSession));
        }
    }
    for (int i = 0; i < PENDING_MAX; i++) {
        if (g_pending[i].active && (t - g_pending[i].since > PENDING_TIMEOUT))
            memset(&g_pending[i], 0, sizeof(PendingPeer));
    }
    xSemaphoreGiveRecursive(stateMutex);
}

// ============================================================================
// 12. HEADER / OUTGOING
// ============================================================================
static void buildHeader(GPacket& p, GMessageType type, uint32_t target,
                        uint8_t totalFrames, uint8_t frameIdx,
                        uint16_t payloadLen, uint8_t flags,
                        uint16_t sessionID, uint32_t seq,
                        uint8_t ttl = DEFAULT_TTL) {
    memset(&p, 0, sizeof(p));
    p.stx = STX_BYTE;
    p.version = G_VERSION;
    p.flags = flags;
    p.senderID = myID;
    p.targetID = target;
    p.sessionID = sessionID;
    p.msgType = (uint8_t)type;
    p.ttl = ttl;
    p.frameIdx = frameIdx;
    p.totalFrames = totalFrames;
    p.payloadLen = payloadLen;
    p.timestamp = nowMs() / 1000;
    p.seq = seq;
    p.originProto = PROTO_LORA;
    p.etx = ETX_BYTE;
}

static OutgoingMessage* outgoingAlloc() {
    for (int i = 0; i < MAX_OUTGOING; i++)
        if (!outgoing[i].active) return &outgoing[i];
    return nullptr;
}

static void outgoingFree(OutgoingMessage* o) { if (o) memset(o, 0, sizeof(OutgoingMessage)); }

static void transmitFrame(GPacket& p);
static bool enqueueTx(const GPacket& p);

static bool sendFrameOfOutgoing(OutgoingMessage* o, uint8_t idx) {
    GPacket p;
    buildHeader(p, (GMessageType)o->msgType, o->targetID,
                o->totalFrames, idx, o->frameLens[idx], o->flags,
                o->sessionID, g_sessionCounter++, DEFAULT_TTL);
    memcpy(p.payload, o->frameData[idx], o->frameLens[idx]);
    transmitFrame(p);
    return true;
}

static void outgoingStart(GMessageType type, uint32_t target,
                          const uint8_t* data, uint16_t len,
                          uint8_t extraFlags, bool needAck = true) {
    OutgoingMessage* o = outgoingAlloc();
    if (!o) { Serial.println("[TX] outgoing full"); return; }
    uint8_t totalFrames = (len == 0) ? 1 : ((len + FRAME_PAYLOAD - 1) / FRAME_PAYLOAD);
    if (totalFrames > MAX_FRAMES) { Serial.println("[TX] msg too long"); return; }

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
}

static void outgoingHandleAck(uint32_t sender, uint16_t session, uint8_t ackMask) {
    for (int i = 0; i < MAX_OUTGOING; i++) {
        OutgoingMessage* o = &outgoing[i];
        if (!o->active || o->sessionID != session || o->targetID != sender) continue;
        o->ackedMask |= ackMask;
        uint8_t need = (1u << o->totalFrames) - 1;
        if ((o->ackedMask & need) == need) outgoingFree(o);
    }
}

static void outgoingTick() {
    unsigned long t = nowMs();
    for (int i = 0; i < MAX_OUTGOING; i++) {
        OutgoingMessage* o = &outgoing[i];
        if (!o->active) continue;
        if (t - o->lastSend < ACK_TIMEOUT_MS) continue;
        if (o->flags & FLAG_NO_RETRY) { outgoingFree(o); continue; }
        uint8_t need = (1u << o->totalFrames) - 1;
        uint8_t missing = need & ~o->ackedMask;
        if (o->retries >= ACK_MAX_RETRIES) { outgoingFree(o); continue; }
        o->retries++;
        for (uint8_t f = 0; f < o->totalFrames; f++)
            if (missing & (1u << f)) sendFrameOfOutgoing(o, f);
        o->lastSend = t;
    }
}

// ============================================================================
// 13. REASSEMBLY
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
    unsigned long t = nowMs();
    for (int i = 0; i < RX_SLOT_COUNT; i++)
        if (rxSlots[i].active && (t - rxSlots[i].lastUpdate) > RX_SESSION_TIMEOUT)
            memset(&rxSlots[i], 0, sizeof(IncomingMessage));
}

static bool protoReady(ProtoType p);
static ProtoType selectProtoFor(uint32_t target);
static void sendVia(ProtoType proto, const GPacket& p);

// ACK via stesso proto d'ingresso (fix handshake via BLE)
static void sendAckBitmap(uint32_t target, uint16_t session, uint8_t mask) {
    if (target == BROADCAST_ID) return;
    uint8_t payload[4];
    payload[0] = (session >> 8) & 0xFF;
    payload[1] = session & 0xFF;
    payload[2] = mask;
    payload[3] = 0;
    GPacket p;
    buildHeader(p, MSG_ACK, target, 1, 0, 4, 0,
                ++g_sessionCounter, g_sessionCounter++, DEFAULT_TTL);
    memcpy(p.payload, payload, 4);
    ProtoType useProto = (ProtoType)g_lastRxProto;
    if (!protoReady(useProto)) useProto = selectProtoFor(target);
    p.originProto = (uint8_t)useProto;
    sendVia(useProto, p);
}

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
// 14. TRASPORTO LoRa
// ============================================================================
static void radioTransmitBlocking(const GPacket& p) {
    uint32_t start = nowMs();
    while (digitalRead(LORA_BUSY) == HIGH && (nowMs() - start) < 500) delay(1);
    radio.standby();
    delay(20);
    int st = radio.transmit((uint8_t*)&p, sizeof(GPacket));
    if (st != RADIOLIB_ERR_NONE) Serial.printf("[TX-LoRa] err=%d\n", st);
    delay(20);
    radio.startReceive();
}

static bool enqueueTx(const GPacket& p) {
    TxRequest r; r.packet = p;
    return xQueueSend(txQueue, &r, pdMS_TO_TICKS(500)) == pdTRUE;
}

static void handleReceivedPacketFromProto(GPacket& p, ProtoType src);

static void radioTask(void*) {
    TxRequest req;
    for (;;) {
        while (xQueueReceive(txQueue, &req, 0) == pdTRUE)
            radioTransmitBlocking(req.packet);

        GPacket p;
        int st = radio.receive((uint8_t*)&p, sizeof(GPacket), 100);
        if (st == RADIOLIB_ERR_NONE) {
            if (p.stx == STX_BYTE && p.etx == ETX_BYTE && p.version == G_VERSION)
                handleReceivedPacketFromProto(p, PROTO_LORA);
            else crc_error_count++;
        } else if (st == RADIOLIB_ERR_CRC_MISMATCH) {
            crc_error_count++;
        }
        recalcInterference();
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

// ============================================================================
// 15. TRASPORTO WiFi (STA + AP + discovery)
// ============================================================================
static IPAddress wifiBroadcastFor(IPAddress ip, IPAddress mask) {
    return IPAddress(ip[0] | ~mask[0], ip[1] | ~mask[1],
                     ip[2] | ~mask[2], ip[3] | ~mask[3]);
}

static void wifiSend(const GPacket& p) {
    if (!g_wifiReady) return;
    if (WiFi.status() == WL_CONNECTED) {
        IPAddress b = wifiBroadcastFor(WiFi.localIP(), WiFi.subnetMask());
        g_udp.beginPacket(b, UDP_DATA_PORT);
        g_udp.write((const uint8_t*)&p, sizeof(GPacket));
        g_udp.endPacket();
    }
    if (WiFi.getMode() & WIFI_MODE_AP) {
        IPAddress b = wifiBroadcastFor(WiFi.softAPIP(), IPAddress(255,255,255,0));
        g_udp.beginPacket(b, UDP_DATA_PORT);
        g_udp.write((const uint8_t*)&p, sizeof(GPacket));
        g_udp.endPacket();
    }
}

static void wifiDataTick() {
    if (!g_wifiReady) return;
    int sz = g_udp.parsePacket();
    if (sz < (int)sizeof(GPacket)) return;
    GPacket p;
    int n = g_udp.read((char*)&p, sizeof(GPacket));
    if (n == (int)sizeof(GPacket) &&
        p.stx == STX_BYTE && p.etx == ETX_BYTE && p.version == G_VERSION) {
        if (p.senderID != myID) handleReceivedPacketFromProto(p, PROTO_WIFI);
    }
}

static void wifiSetupSync() {
    WiFi.persistent(false);
    WiFi.setSleep(false);
    WiFi.mode(WIFI_AP);
    delay(200);

    char mySSID[24];
    sprintf(mySSID, "%s%08X", WIFI_SSID_PREFIX, myID);
    WiFi.softAP(mySSID, WIFI_PASSWORD);
    delay(500);

    Serial.printf("[WiFi] AP %s -> %s\n", mySSID, WiFi.softAPIP().toString().c_str());

    g_udp.begin(UDP_DATA_PORT);
    g_wifiReady = true;
    Serial.printf("[WiFi] UDP data port %d ready\n", UDP_DATA_PORT);
}

// Retry periodico STA (ogni 60s) — non blocca
static unsigned long g_lastStaTry = 0;
static void wifiStaTry() {
    return; //disabilitato per ora: causava stack overflow nel loopTask
    if (!g_wifiReady) return;
    if (WiFi.status() == WL_CONNECTED) return;
    if (nowMs() - g_lastStaTry < STA_RETRY_INTERVAL_MS) return;
    g_lastStaTry = nowMs();

    char mySSID[24];
    sprintf(mySSID, "%s%08X", WIFI_SSID_PREFIX, myID);

    Serial.println("[WiFi] STA retry scan...");
    int n = WiFi.scanNetworks();
    if (n <= 0) { Serial.println("[WiFi] scan fail"); return; }

    String bestSSID;
    int bestRSSI = -1000;
    for (int i = 0; i < n; i++) {
        String ssid = WiFi.SSID(i);
        if (ssid.startsWith(WIFI_SSID_PREFIX) && ssid != mySSID) {
            if (WiFi.RSSI(i) > bestRSSI) {
                bestRSSI = WiFi.RSSI(i);
                bestSSID = ssid;
            }
        }
    }
    WiFi.scanDelete();

    if (bestSSID.length() == 0) {
        Serial.println("[WiFi] nessuna QORLYN trovata");
        return;
    }

    Serial.printf("[WiFi] STA retry join %s (rssi=%d)\n", bestSSID.c_str(), bestRSSI);
    if (WiFi.getMode() != WIFI_AP_STA) {
        WiFi.mode(WIFI_AP_STA);
        delay(200);
    }
    WiFi.begin(bestSSID.c_str(), WIFI_PASSWORD);
    // Non blocca: la connessione avverrà in background
}

// ============================================================================
// 16. TRASPORTO BLE (peripheral + central)
// ============================================================================
class QorlynBleServerCB : public BLEServerCallbacks {
    void onConnect(BLEServer*) override {
        g_bleCentralConnected = true;
        Serial.println("[BLE] central connected");
    }
    void onDisconnect(BLEServer*) override {
        g_bleCentralConnected = false;
        Serial.println("[BLE] central disconnected");
        BLEDevice::startAdvertising();
    }
};

static void bleHandleIncoming(const uint8_t* data, size_t len);

class QorlynBleRxCB : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* c) override {
        String v = c->getValue();
        if (v.length() < 1) return;
        bleHandleIncoming((const uint8_t*)v.c_str(), v.length());
    }
};

static void bleSendPeripheral(const uint8_t* data, size_t len) {
    if (!g_btReady) return;
    if (!g_bleCentralConnected || !g_bleTxChar) return;
    size_t off = 0;
    while (off < len) {
        size_t chunk = len - off;
        if (chunk > BLE_MTU_PAYLOAD) chunk = BLE_MTU_PAYLOAD;
        g_bleTxChar->setValue((uint8_t*)(data + off), chunk);
        g_bleTxChar->notify();
        off += chunk;
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

static void bleCentralNotifyCB(BLERemoteCharacteristic* c, uint8_t* data, size_t len, bool isNotify) {
    (void)c; (void)isNotify;
    bleHandleIncoming(data, len);
}

static void bleCentralScanTask(void*) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(BLE_SCAN_INTERVAL_MS));
        if (!g_btReady) continue;
        if (!g_bleScanEnabled) continue;

        int active = 0;
        for (int i = 0; i < BLE_MAX_CENTRALS; i++) if (g_bleCentrals[i].active) active++;
        if (active >= BLE_MAX_CENTRALS) continue;

        BLEScan* scan = BLEDevice::getScan();
        scan->setActiveScan(true);
        BLEScanResults* res = scan->start(3, false);
        if (!res) continue;

        for (int i = 0; i < res->getCount(); i++) {
            BLEAdvertisedDevice dev = res->getDevice(i);
            if (!dev.haveName()) continue;
            String name = dev.getName().c_str();
            if (!name.startsWith(BLE_DEVICE_PREFIX)) continue;

            int dash = name.lastIndexOf('-');
            if (dash < 0) continue;
            uint32_t peerID = strtoul(name.substring(dash + 1).c_str(), nullptr, 16);
            if (peerID == 0 || peerID == myID) continue;

            // Elezione BLE: l'ID piu' alto si connette come central
            // (basta che solo uno dei due lati tenti la connessione)
            if (peerID > myID) continue;

            bool dup = false;
            for (int j = 0; j < BLE_MAX_CENTRALS; j++)
                if (g_bleCentrals[j].active && g_bleCentrals[j].peerID == peerID) { dup = true; break; }
            if (dup) continue;

            int slot = -1;
            for (int j = 0; j < BLE_MAX_CENTRALS; j++)
                if (!g_bleCentrals[j].active) { slot = j; break; }
            if (slot < 0) break;

            Serial.printf("[BLE] connecting to %s\n", name.c_str());
            BLEClient* cli = BLEDevice::createClient();
            if (!cli->connect(&dev)) {
                Serial.println("[BLE] connect fail");
                delete cli;
                continue;
            }
            BLERemoteService* svc = cli->getService(BLE_SVC_UUID);
            if (!svc) { cli->disconnect(); delete cli; continue; }
            BLERemoteCharacteristic* txr = svc->getCharacteristic(BLE_TX_CHAR_UUID);
            BLERemoteCharacteristic* rxr = svc->getCharacteristic(BLE_RX_CHAR_UUID);
            if (!txr || !rxr) { cli->disconnect(); delete cli; continue; }
            if (txr->canNotify()) txr->registerForNotify(bleCentralNotifyCB);

            g_bleCentrals[slot].active = true;
            g_bleCentrals[slot].client = cli;
            g_bleCentrals[slot].txChar = txr;
            g_bleCentrals[slot].rxChar = rxr;
            g_bleCentrals[slot].peerID = peerID;
            g_bleCentrals[slot].lastSeen = nowMs();
            Serial.printf("[BLE] central linked to %08X\n", peerID);
        }
    }
}

static void bleSetup() {
    snprintf(g_bleName, sizeof(g_bleName), "%s-%08X", BLE_DEVICE_PREFIX, myID);
    BLEDevice::init(g_bleName);
    BLEDevice::setMTU(517);
    g_bleServer = BLEDevice::createServer();
    g_bleServer->setCallbacks(new QorlynBleServerCB());

    BLEService* svc = g_bleServer->createService(BLE_SVC_UUID);

    g_bleRxChar = svc->createCharacteristic(BLE_RX_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
    g_bleRxChar->setCallbacks(new QorlynBleRxCB());

    g_bleTxChar = svc->createCharacteristic(BLE_TX_CHAR_UUID,
        BLECharacteristic::PROPERTY_NOTIFY);
    g_bleTxChar->addDescriptor(new BLE2902());

    svc->start();

    BLEAdvertising* adv = BLEDevice::getAdvertising();
    adv->addServiceUUID(BLE_SVC_UUID);
    adv->setScanResponse(true);
    BLEDevice::startAdvertising();

    g_btReady = true;
    Serial.println("[BLE] peripheral ready");

    xTaskCreatePinnedToCore(bleCentralScanTask, "bleScan", 4096, nullptr, 1, &bleTaskHandle, 0);
}

// ============================================================================
// 17. BLE chunking
// ============================================================================
static uint8_t  s_bleRxBuf[sizeof(GPacket) * 2];
static size_t   s_bleRxLen = 0;

static void bleHandleIncoming(const uint8_t* data, size_t len) {
    if (len == sizeof(GPacket)) {
        GPacket p;
        memcpy(&p, data, sizeof(GPacket));
        if (p.stx == STX_BYTE && p.etx == ETX_BYTE && p.version == G_VERSION
            && p.senderID != myID) {
            // Non processare qui: queue al mainTask (stack 16 KB)
            xQueueSend(bleRxQueue, &p, 0);
        }
        return;
    }
    if (s_bleRxLen + len <= sizeof(s_bleRxBuf)) {
        memcpy(s_bleRxBuf + s_bleRxLen, data, len);
        s_bleRxLen += len;
        if (s_bleRxLen >= sizeof(GPacket)) {
            GPacket p;
            memcpy(&p, s_bleRxBuf, sizeof(GPacket));
            s_bleRxLen = 0;
            if (p.stx == STX_BYTE && p.etx == ETX_BYTE && p.version == G_VERSION
                && p.senderID != myID) {
                xQueueSend(bleRxQueue, &p, 0);
            }
        }
    } else {
        s_bleRxLen = 0;
    }
}

static void bleSend(const GPacket& p) {
    if (!g_btReady) return;
    if (g_bleCentralConnected && g_bleTxChar) {
        bleSendPeripheral((const uint8_t*)&p, sizeof(GPacket));
    }
    for (int i = 0; i < BLE_MAX_CENTRALS; i++) {
        if (!g_bleCentrals[i].active) continue;
        if (p.targetID != BROADCAST_ID && g_bleCentrals[i].peerID != p.targetID) continue;
        BLERemoteCharacteristic* rxr = g_bleCentrals[i].rxChar;
        if (!rxr) continue;
        size_t off = 0;
        while (off < sizeof(GPacket)) {
            size_t chunk = sizeof(GPacket) - off;
            if (chunk > BLE_MTU_PAYLOAD) chunk = BLE_MTU_PAYLOAD;
            rxr->writeValue((uint8_t*)((const uint8_t*)&p + off), chunk, false);
            off += chunk;
        }
        g_bleCentrals[i].lastSeen = nowMs();
        if (p.targetID != BROADCAST_ID) return;
    }
}

// ============================================================================
// 18. SELEZIONE TRASPORTO + BRIDGING
// ============================================================================
static bool protoReady(ProtoType p) {
    switch (p) {
        case PROTO_LORA: return g_loraReady;
        case PROTO_WIFI: return g_wifiReady;
        case PROTO_BT:   return g_btReady && (g_bleCentralConnected || g_bleCentrals[0].active || g_bleCentrals[1].active);
    }
    return false;
}

static ProtoType selectProtoFor(uint32_t target) {
    if (target != BROADCAST_ID) {
        PeerSession* ps = peerFind(target);
        if (ps && ps->state != PEER_NONE) {
            ProtoType pref = (ProtoType)ps->lastSeenProto;
            if (protoReady(pref)) return pref;
        }
    }
    if (g_loraReady) return PROTO_LORA;
    if (g_wifiReady) return PROTO_WIFI;
    if (protoReady(PROTO_BT)) return PROTO_BT;
    return PROTO_LORA;
}

static void sendVia(ProtoType proto, const GPacket& p) {
    switch (proto) {
        case PROTO_LORA: enqueueTx(p); break;
        case PROTO_WIFI: wifiSend(p); break;
        case PROTO_BT:   bleSend(p); break;
    }
}

static void transmitFrame(GPacket& p) {
    // Handshake (PK e CT): broadcast su tutti i proto attivi per affidabilita'.
    // Il dedup sul ricevente scartera' i duplicati.
    if (p.msgType == MSG_KYBER_PUBKEY || p.msgType == MSG_KYBER_CIPHERTEXT) {
        bool sent = false;
        for (uint8_t i = 0; i < 3; i++) {
            ProtoType pt = (ProtoType)i;
            if (!protoReady(pt)) continue;
            GPacket copy = p;
            copy.originProto = (uint8_t)pt;
            sendVia(pt, copy);
            sent = true;
        }
        if (!sent) Serial.println("[TX] handshake: nessun proto ready");
        return;
    }
    ProtoType useProto = selectProtoFor(p.targetID);
    p.originProto = (uint8_t)useProto;
    if (!protoReady(useProto)) {
        Serial.printf("[TX] proto %s non ready\n", protoName(useProto));
    }
    sendVia(useProto, p);
}

// ============================================================================
// 19. ONION ROUTING (X25519 effimero per-hop)
// ============================================================================
static void onionTrackerSweep() {
    unsigned long t = nowMs();
    for (int i = 0; i < ONION_TRACKER_SIZE; i++)
        if (g_onionTrackers[i].active && (t - g_onionTrackers[i].sentAt) > ONION_TRACKER_TTL)
            g_onionTrackers[i].active = false;
}

static uint32_t onionTrackerAdd(uint32_t targetID) {
    int slot = -1;
    for (int i = 0; i < ONION_TRACKER_SIZE; i++)
        if (!g_onionTrackers[i].active) { slot = i; break; }
    if (slot < 0) {
        unsigned long oldestT = ULONG_MAX;
        for (int i = 0; i < ONION_TRACKER_SIZE; i++)
            if (g_onionTrackers[i].sentAt < oldestT) { oldestT = g_onionTrackers[i].sentAt; slot = i; }
    }
    uint32_t token;
    do { token = esp_random(); } while (token == 0);
    g_onionTrackers[slot].active = true;
    g_onionTrackers[slot].token = token;
    g_onionTrackers[slot].targetID = targetID;
    g_onionTrackers[slot].sentAt = nowMs();
    return token;
}

static bool onionTrackerConsume(uint32_t token) {
    for (int i = 0; i < ONION_TRACKER_SIZE; i++) {
        if (g_onionTrackers[i].active && g_onionTrackers[i].token == token) {
            g_onionTrackers[i].active = false;
            return true;
        }
    }
    return false;
}

static void dispatchOnionDelivered(uint32_t origSender, uint8_t type,
                                   uint8_t* data, uint16_t len,
                                   bool ackReq, uint32_t ackToken);

static bool onionBuildLayer(const uint8_t* hop_pk,
                            const uint8_t* plaintext, uint16_t ptLen,
                            uint8_t* out, uint16_t& outLen) {
    if (ptLen + ONION_LAYER_OVERHEAD > ONION_BUF) return false;
    uint8_t eph_sk[X25519_KEY_SIZE];
    uint8_t eph_pk[X25519_KEY_SIZE];
    if (!x25519GenKeypair(eph_sk, eph_pk)) return false;
    uint8_t K[32];
    if (!x25519Shared(eph_sk, hop_pk, K)) return false;

    uint8_t aad[X25519_KEY_SIZE];
    memcpy(aad, eph_pk, X25519_KEY_SIZE);

    uint8_t ct[ONION_BUF];
    uint16_t ctLen = 0;
    if (!x25519Encrypt(K, aad, X25519_KEY_SIZE, plaintext, ptLen, ct, ctLen)) return false;

    memcpy(out, eph_pk, X25519_KEY_SIZE);
    memcpy(out + X25519_KEY_SIZE, ct, ctLen);
    outLen = X25519_KEY_SIZE + ctLen;
    return true;
}

static bool onionSend(GMessageType finalType, uint32_t target,
                      const uint8_t* data, uint16_t dataLen,
                      const uint32_t* hops, uint8_t nHops,
                      bool ackReq) {
    uint8_t pathLen = nHops + 1;
    if (pathLen > ONION_MAX_HOPS) return false;

    uint32_t path[ONION_MAX_HOPS];
    for (uint8_t i = 0; i < nHops; i++) path[i] = hops[i];
    path[nHops] = target;

    for (uint8_t i = 0; i < pathLen; i++) {
        if (path[i] == BROADCAST_ID) return false;
        if (path[i] == myID) continue;  // siamo noi stessi, skip check
        PeerSession* ps = peerFind(path[i]);
        if (!ps || !ps->has_x25519_pk) {
            Serial.printf("[ONION] no x25519 pk for %08X\n", path[i]);
            return false;
        }
    }

    uint16_t overhead = ONION_LAYER_OVERHEAD * pathLen + ONION_INNER_HDR;
    if (dataLen + overhead > MAX_MESSAGE_BYTES) return false;

    uint32_t ackToken = ackReq ? onionTrackerAdd(target) : 0;

    {
        uint16_t i = 0;
        uint16_t innerLen = ONION_INNER_HDR + dataLen;
        uint8_t p0[ONION_BUF];
        p0[i++] = 0; p0[i++] = 0; p0[i++] = 0; p0[i++] = 0;
        p0[i++] = (uint8_t)finalType;
        p0[i++] = ackReq ? ONION_FLAG_ACK_REQ : 0;
        p0[i++] = (innerLen >> 8) & 0xFF;
        p0[i++] = innerLen & 0xFF;
        p0[i++] = (myID >> 24) & 0xFF;
        p0[i++] = (myID >> 16) & 0xFF;
        p0[i++] = (myID >>  8) & 0xFF;
        p0[i++] = (myID      ) & 0xFF;
        p0[i++] = (ackToken >> 24) & 0xFF;
        p0[i++] = (ackToken >> 16) & 0xFF;
        p0[i++] = (ackToken >>  8) & 0xFF;
        p0[i++] = (ackToken      ) & 0xFF;
        if (dataLen) { memcpy(p0 + i, data, dataLen); i += dataLen; }

        const uint8_t* target_pk;
        if (target == myID) {
            target_pk = g_x25519_pk;
        } else {
            PeerSession* tps = peerFind(target);
            if (!tps) return false;
            target_pk = tps->x25519_pk;
        }
        uint16_t curLen = 0;
        if (!onionBuildLayer(target_pk, p0, i, s_onionA, curLen)) return false;
        uint16_t bufLen = curLen;
        uint8_t* cur = s_onionA;
        uint8_t* nxt = s_onionB;

        for (int k = (int)nHops - 1; k >= 0; k--) {
            const uint8_t* hop_pk;
            if (path[k] == myID) {
                hop_pk = g_x25519_pk;
            } else {
                PeerSession* ps = peerFind(path[k]);
                if (!ps) return false;
                hop_pk = ps->x25519_pk;
            }
            uint32_t nextHop = path[k + 1];

            uint8_t plain[ONION_BUF];
            uint16_t j = 0;
            plain[j++] = (nextHop >> 24) & 0xFF;
            plain[j++] = (nextHop >> 16) & 0xFF;
            plain[j++] = (nextHop >>  8) & 0xFF;
            plain[j++] = (nextHop      ) & 0xFF;
            plain[j++] = (uint8_t)MSG_ONION;
            plain[j++] = 0;
            plain[j++] = (bufLen >> 8) & 0xFF;
            plain[j++] = bufLen & 0xFF;
            memcpy(plain + j, cur, bufLen); j += bufLen;

            uint16_t outLen = 0;
            if (!onionBuildLayer(hop_pk, plain, j, nxt, outLen)) return false;
            uint8_t* t = cur; cur = nxt; nxt = t;
            bufLen = outLen;
        }

        Serial.printf("[ONION] send path=%u target=%08X len=%u ack=%d\n",
                      pathLen, target, bufLen, ackReq ? 1 : 0);

        uint32_t firstHop = (nHops > 0) ? path[0] : target;
        outgoingStart(MSG_ONION, firstHop, cur, bufLen,
                      FLAG_RELAY_OK | FLAG_NO_RETRY, false);
    }
    return true;
}

static void handleOnionLayer(uint32_t fromHop, uint8_t* data, uint16_t len) {
    if (!g_x25519_ready) return;
    if (len < ONION_EPH_PUB + 28) return;

    const uint8_t* eph_pub = data;
    const uint8_t* ct = data + ONION_EPH_PUB;
    uint16_t ctLen = len - ONION_EPH_PUB;

    uint8_t K[32];
    if (!x25519Shared(g_x25519_sk, eph_pub, K)) return;

    uint8_t aad[X25519_KEY_SIZE];
    memcpy(aad, eph_pub, X25519_KEY_SIZE);

    uint16_t plainLen = 0;
    if (!x25519Decrypt(K, aad, X25519_KEY_SIZE, ct, ctLen, s_onionPlain, plainLen)) {
        Serial.println("[ONION] decrypt fail");
        return;
    }
    if (plainLen < ONION_LAYER_HDR) return;

    uint32_t nextHop = ((uint32_t)s_onionPlain[0] << 24) | ((uint32_t)s_onionPlain[1] << 16)
                     | ((uint32_t)s_onionPlain[2] <<  8) |  (uint32_t)s_onionPlain[3];
    uint8_t  nextType = s_onionPlain[4];
    uint8_t  nextFlags = s_onionPlain[5];
    uint16_t innerLen = ((uint16_t)s_onionPlain[6] << 8) | s_onionPlain[7];
    uint8_t* inner    = s_onionPlain + ONION_LAYER_HDR;
    if (innerLen > plainLen - ONION_LAYER_HDR) return;

    if (nextHop == 0) {
        if (innerLen < ONION_INNER_HDR) return;
        uint32_t origSender = ((uint32_t)inner[0] << 24) | ((uint32_t)inner[1] << 16)
                            | ((uint32_t)inner[2] <<  8) |  (uint32_t)inner[3];
        uint32_t token = ((uint32_t)inner[4] << 24) | ((uint32_t)inner[5] << 16)
                       | ((uint32_t)inner[6] <<  8) |  (uint32_t)inner[7];
        bool ackReq = (nextFlags & ONION_FLAG_ACK_REQ) != 0;
        uint8_t* userData = inner + ONION_INNER_HDR;
        uint16_t userLen = innerLen - ONION_INNER_HDR;

        if (nextType == MSG_ONION || nextType == MSG_ACK || nextType == MSG_ONION_ACK) return;

        dispatchOnionDelivered(origSender, nextType, userData, userLen, ackReq, token);
        Serial.printf("[ONION] delivered from %08X len=%u\n", origSender, userLen);
    } else {
        if (nextType != MSG_ONION) return;

        outgoingStart(MSG_ONION, nextHop, inner, innerLen,
                      FLAG_RELAY_OK | FLAG_NO_RETRY, false);
        Serial.printf("[ONION] relay -> %08X (%u B)\n", nextHop, innerLen);
    }
}

// ============================================================================
// 20. ONION DELIVERY + ACK end-to-end
// ============================================================================
static void dispatchOnionDelivered(uint32_t origSender, uint8_t type,
                                   uint8_t* data, uint16_t len,
                                   bool ackReq, uint32_t ackToken) {
    if (ackReq && g_x25519_ready) {
        PeerSession* sPeer = peerFind(origSender);
        if (sPeer && sPeer->has_x25519_pk) {
            uint8_t eph_sk[X25519_KEY_SIZE];
            uint8_t eph_pk[X25519_KEY_SIZE];
            if (x25519GenKeypair(eph_sk, eph_pk)) {
                uint8_t K[32];
                if (x25519Shared(eph_sk, sPeer->x25519_pk, K)) {
                    uint8_t token[4] = {
                        (uint8_t)(ackToken >> 24), (uint8_t)(ackToken >> 16),
                        (uint8_t)(ackToken >>  8), (uint8_t)(ackToken      )
                    };
                    uint8_t aad[X25519_KEY_SIZE];
                    memcpy(aad, eph_pk, X25519_KEY_SIZE);
                    uint8_t ct[64]; uint16_t ctLen = 0;
                    if (x25519Encrypt(K, aad, X25519_KEY_SIZE, token, 4, ct, ctLen)) {
                        uint8_t payload[X25519_KEY_SIZE + 64];
                        memcpy(payload, eph_pk, X25519_KEY_SIZE);
                        memcpy(payload + X25519_KEY_SIZE, ct, ctLen);
                        uint16_t plen = X25519_KEY_SIZE + ctLen;

                        GPacket p;
                        buildHeader(p, MSG_ONION_ACK, origSender, 1, 0, plen,
                                    FLAG_RELAY_OK | FLAG_X25519_ENC | FLAG_ACK_REQ,
                                    ++g_sessionCounter, g_sessionCounter++, DEFAULT_TTL);
                        memcpy(p.payload, payload, plen);
                        transmitFrame(p);
                        Serial.printf("[ONION] ACK -> %08X\n", origSender);
                    }
                }
            }
        }
    }

    switch (type) {
        case MSG_HEARTBEAT:
            Serial.printf("[ONION] HB from %08X\n", origSender);
            break;
        case MSG_SOS_MEDIC:
            Serial.printf("[ONION] !!! SOS from %08X !!!\n", origSender);
            updateUI("SOS", "SOS (onion)");
            break;
        case MSG_CHAT: {
            char buf[MAX_MESSAGE_BYTES + 1];
            uint16_t n = len > MAX_MESSAGE_BYTES ? MAX_MESSAGE_BYTES : len;
            memcpy(buf, data, n); buf[n] = '\0';
            Serial.printf("[ONION] Chat %08X: %s\n", origSender, buf);
            updateUI("CHAT", buf);
            break;
        }
        default:
            Serial.printf("[ONION] type 0x%02X len=%u from %08X\n", type, len, origSender);
            break;
    }
}

static void handleOnionAckPacket(GPacket& frame, uint8_t* data, uint16_t len) {
    if (!g_x25519_ready) return;
    if (len < X25519_KEY_SIZE + 28) return;
    const uint8_t* eph_pub = data;
    const uint8_t* ct = data + X25519_KEY_SIZE;
    uint16_t ctLen = len - X25519_KEY_SIZE;

    uint8_t K[32];
    if (!x25519Shared(g_x25519_sk, eph_pub, K)) return;
    uint8_t aad[X25519_KEY_SIZE];
    memcpy(aad, eph_pub, X25519_KEY_SIZE);

    uint8_t token[4]; uint16_t outLen = 0;
    if (!x25519Decrypt(K, aad, X25519_KEY_SIZE, ct, ctLen, token, outLen)) return;
    if (outLen < 4) return;
    uint32_t tok = ((uint32_t)token[0] << 24) | ((uint32_t)token[1] << 16)
                 | ((uint32_t)token[2] <<  8) |  (uint32_t)token[3];
    if (onionTrackerConsume(tok)) {
        Serial.printf("[ONION] *** ACK E2E da %08X ***\n", frame.senderID);
        updateUI("ONION ACK", "delivered");
    }
}

// ============================================================================
// 21. KEY EXCHANGE + HEARTBEAT X25519
// ============================================================================
static void sendMyX25519To(uint32_t peerID) {
    x25519EnsureKeypair();
    if (!g_x25519_ready) return;
    GPacket hb;
    buildHeader(hb, MSG_HEARTBEAT, peerID, 1, 0, X25519_KEY_SIZE,
                FLAG_RELAY_OK,
                ++g_sessionCounter, g_sessionCounter++, DEFAULT_TTL);
    memcpy(hb.payload, g_x25519_pk, X25519_KEY_SIZE);
    transmitFrame(hb);
    Serial.printf("[X25519] sent pk to %08X\n", peerID);
}

static void handleHeartbeatPayload(const uint8_t* data, uint16_t len, uint32_t sender) {
    if (len < X25519_KEY_SIZE) return;
    PeerSession* ps = peerFind(sender);
    if (!ps || ps->state == PEER_NONE) return;
    memcpy(ps->x25519_pk, data, X25519_KEY_SIZE);
    ps->has_x25519_pk = true;
    Serial.printf("[X25519] got pk from %08X\n", sender);
}

static void handleKeyExchangePubKey(const uint8_t* data, uint16_t len, uint32_t remoteID) {
    if (len != KYBER_PK_SIZE) return;
    if (isBlocked(remoteID)) return;
    PeerSession* ps = peerFind(remoteID);
    if (ps && ps->state == PEER_ESTABLISHED) return;

    unsigned long t = nowMs();
    if (ps && (t - ps->last_request_time < REQUEST_MIN_INTERVAL)) return;

    if (!ps) {
        ps = peerGetOrCreate(remoteID);
        if (!ps) return;
    }
    ps->last_request_time = t;
    ps->last_activity = t;
    ps->lastSeenProto = g_lastRxProto;

    PendingPeer* pp = pendingFind(remoteID);
    if (!pp) pp = pendingAlloc();
    if (!pp) return;
    pp->active = true;
    pp->peerID = remoteID;
    memcpy(pp->pk, data, KYBER_PK_SIZE);
    pp->since = t;

    bool accept = false;
    switch (g_policy) {
        case POLICY_AUTO:      accept = true; break;
        case POLICY_WHITELIST: accept = isWhitelisted(remoteID); break;
        case POLICY_HYBRID:    accept = (countEstablished() < HYBRID_AUTO_LIMIT); break;
        case POLICY_MANUAL:    accept = false; break;
    }
    ps->state = PEER_PENDING_IN;

    if (accept) {
        static uint8_t ct[KYBER_CT_SIZE];
        uint8_t shared[32];
        pqc_kyber512_encapsulate(ct, shared, pp->pk);
        memcpy(ps->shared_secret, shared, 32);
        ps->state = PEER_ESTABLISHED;
        ps->tx_seq = 1;
        ps->last_rx_seq = 0;
        ps->lastSeenProto = g_lastRxProto;
        pendingFree(remoteID);
        outgoingStart(MSG_KYBER_CIPHERTEXT, remoteID, ct, KYBER_CT_SIZE, FLAG_RELAY_OK, true);
        Serial.printf("[KYBER] auto-accept %08X\n", remoteID);
        updateUI("SECURE", "auto");
        sendMyX25519To(remoteID);
    } else {
        Serial.printf("[KYBER] PENDING %08X\n", remoteID);
        updateUI("PENDING", "richiesta peer");
    }
}

static void handleKeyExchangeCiphertext(const uint8_t* data, uint16_t len, uint32_t remoteID) {
    if (len != KYBER_CT_SIZE) return;
    if (!g_kyber_ready) return;
    if (isBlocked(remoteID)) return;
    PeerSession* ps = peerFind(remoteID);
    if (!ps || ps->state != PEER_PENDING_OUT) return;
    uint8_t shared[32];
    pqc_kyber512_decapsulate(shared, data, g_kyber_sk);
    memcpy(ps->shared_secret, shared, 32);
    ps->state = PEER_ESTABLISHED;
    ps->tx_seq = 1;
    ps->last_rx_seq = 0;
    ps->last_activity = nowMs();
    ps->lastSeenProto = g_lastRxProto;
    Serial.printf("[KYBER] ESTABLISHED with %08X\n", remoteID);
    updateUI("SECURE", "sessione attiva");
    sendMyX25519To(remoteID);
}

static bool initiateHandshake(uint32_t target) {
    if (target == BROADCAST_ID || target == myID) {
        Serial.println(F("[KYBER] target non valido"));
        return false;
    }
    if (isBlocked(target)) {
        Serial.printf("[KYBER] target %08X bloccato\n", target);
        return false;
    }

    PeerSession* ps = peerFind(target);
    if (ps) {
        if (ps->state == PEER_ESTABLISHED) {
            Serial.printf("[KYBER] %08X gia' established, skip\n", target);
            return false;
        }
        if (ps->state == PEER_PENDING_OUT) {
            Serial.printf("[KYBER] %08X gia' pending-out, skip\n", target);
            return false;
        }
    }
    if (!ps) {
        ps = peerGetOrCreate(target);
        if (!ps) return false;
    } else {
        pendingFree(target);
    }
    ps->state = PEER_PENDING_OUT;
    ps->last_activity = nowMs();
    ps->last_request_time = nowMs();

    kyberEnsureKeypair();
    Serial.printf("[KYBER] init -> %08X\n", target);
    outgoingStart(MSG_KYBER_PUBKEY, target, g_kyber_pk, KYBER_PK_SIZE, FLAG_RELAY_OK, true);
    updateUI("HANDSHAKE", "attesa CT");
    return true;
}

static bool peerAcceptPending(uint32_t peerID) {
    PendingPeer* pp = pendingFind(peerID);
    PeerSession* ps = peerFind(peerID);
    if (!pp || !ps || ps->state != PEER_PENDING_IN) return false;

    static uint8_t ct[KYBER_CT_SIZE];
    uint8_t shared[32];
    pqc_kyber512_encapsulate(ct, shared, pp->pk);
    memcpy(ps->shared_secret, shared, 32);
    ps->state = PEER_ESTABLISHED;
    ps->tx_seq = 1;
    ps->last_rx_seq = 0;
    ps->last_activity = nowMs();
    pendingFree(peerID);
    outgoingStart(MSG_KYBER_CIPHERTEXT, peerID, ct, KYBER_CT_SIZE, FLAG_RELAY_OK, true);
    Serial.printf("[KYBER] ACCEPT %08X\n", peerID);
    updateUI("ACCEPT", "peer OK");
    sendMyX25519To(peerID);
    return true;
}

static bool peerRejectPending(uint32_t peerID) {
    pendingFree(peerID);
    PeerSession* ps = peerFind(peerID);
    if (ps && ps->state == PEER_PENDING_IN) memset(ps, 0, sizeof(PeerSession));

    GPacket p;
    buildHeader(p, MSG_KYBER_REJECT, peerID, 1, 0, 0,
                FLAG_RELAY_OK, ++g_sessionCounter, g_sessionCounter++, DEFAULT_TTL);
    transmitFrame(p);
    Serial.printf("[KYBER] REJECT %08X\n", peerID);
    return true;
}

// ============================================================================
// 22. FINAL DISPATCH
// ============================================================================
static void processFinalMessage(GPacket& frame, uint8_t* data, uint16_t len) {
    uint8_t  type   = frame.msgType;
    uint32_t sender = frame.senderID;

    if (isBlocked(sender)) return;

    if (type == MSG_ONION) { handleOnionLayer(sender, data, len); return; }
    if (type == MSG_ONION_ACK) { handleOnionAckPacket(frame, data, len); return; }
    if (type == MSG_KYBER_PUBKEY) { handleKeyExchangePubKey(data, len, sender); return; }
    if (type == MSG_KYBER_CIPHERTEXT) { handleKeyExchangeCiphertext(data, len, sender); return; }
    if (type == MSG_KYBER_REJECT) {
        PeerSession* ps = peerFind(sender);
        if (ps && ps->state == PEER_PENDING_OUT) memset(ps, 0, sizeof(PeerSession));
        updateUI("REJECT", "peer rifiutato");
        return;
    }
    if (type == MSG_ACK) {
        if (len < 3) return;
        uint16_t sess = (data[0] << 8) | data[1];
        uint8_t  mask = data[2];
        outgoingHandleAck(sender, sess, mask);
        return;
    }
    if (type == MSG_NACK) return;

    if (type == MSG_HEARTBEAT) {
        if (len >= X25519_KEY_SIZE) handleHeartbeatPayload(data, len, sender);
        return;
    }

    const uint8_t* plain = data;
    uint16_t plainLen = len;
    static uint8_t tmp[MAX_MESSAGE_BYTES];

    if (frame.flags & FLAG_ENCRYPTED) {
        PeerSession* ps = peerFind(sender);
        if (!ps || ps->state != PEER_ESTABLISHED) return;
        uint16_t outLen = 0;
        if (!decryptGCM_AAD(ps->shared_secret, frame, data, len, tmp, outLen)) return;
        if (frame.seq <= ps->last_rx_seq) return;
        ps->last_rx_seq = frame.seq;
        ps->last_activity = nowMs();
        plain = tmp; plainLen = outLen;
    }

    switch (type) {
        case MSG_SOS_MEDIC:
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
    }

    // Warning plaintext: loggato solo su seriale, NON sovrascrive l'OLED
    if (!(frame.flags & FLAG_ENCRYPTED) && (frame.flags & FLAG_PLAINTEXT_WARN)
        && frame.targetID != BROADCAST_ID) {
        Serial.printf("[RX] *** PLAINTEXT da %08X ***\n", sender);
    }
}

// ============================================================================
// 23. RICEZIONE
// ============================================================================
static void handleReceivedPacketFromProto(GPacket& p, ProtoType src) {
    g_lastRxProto = (uint8_t)src;

    if (p.targetID != BROADCAST_ID && p.senderID != myID && isBlocked(p.senderID)) return;

    if (p.senderID != myID && (ProtoType)p.originProto == src) {
        PeerSession* ps = peerFind(p.senderID);
        if (ps && ps->state != PEER_NONE) ps->lastSeenProto = (uint8_t)src;
    }

    if (dedupCheckAndInsert(p.senderID, p.sessionID, p.frameIdx)) return;

    bool isForMe     = (p.targetID == myID);
    bool isBroadcast = (p.targetID == BROADCAST_ID);
    bool forMe       = isForMe || isBroadcast;
    bool relayable   = (p.flags & FLAG_RELAY_OK) && p.ttl > 0 && p.senderID != myID;

    if (!forMe) foreign_packets_count++;

    // Log utile per debug: pacchetti per me (non ACK/HB) da che proto arrivano
    if (forMe && !isBroadcast && p.msgType != MSG_ACK && p.msgType != MSG_HEARTBEAT) {
        Serial.printf("[RX] %s from %08X type=0x%02X frame=%u/%u\n",
                      protoName(src), p.senderID, p.msgType,
                      (unsigned)p.frameIdx + 1, (unsigned)p.totalFrames);
    }

    if (relayable && p.targetID != myID && p.msgType != MSG_ONION) {
        GPacket r = p;
        r.ttl--;
        ProtoType outProto = src;
        if (p.targetID != BROADCAST_ID) {
            ProtoType preferred = selectProtoFor(p.targetID);
            if (preferred != src && protoReady(preferred)) {
                outProto = preferred;
                Serial.printf("[RELAY] bridge %s -> %s for %08X\n",
                              protoName(src), protoName(outProto), p.targetID);
            }
        }
        r.originProto = (uint8_t)outProto;
        sendVia(outProto, r);
    }
    if (forMe) {
        updateUI("RX", protoName(src), 0, 0);
        handlePacketReassembly(p);
    }
    lastActivity = nowMs();
}

// ============================================================================
// 24. SEND SECURE OR PLAIN
// ============================================================================
static void sendSecureOrPlain(GMessageType type, uint32_t target,
                              const uint8_t* data, uint16_t len) {
    PeerSession* ps = peerFind(target);
    bool doEnc = (ps && ps->state == PEER_ESTABLISHED && target != BROADCAST_ID);

    if (doEnc && len + ENC_OVERHEAD <= FRAME_PAYLOAD) {
        uint8_t flags = FLAG_RELAY_OK | FLAG_ENCRYPTED;
        if (target != BROADCAST_ID) flags |= FLAG_ACK_REQ;
        uint16_t sessionID = ++g_sessionCounter;
        uint32_t seq = ps->tx_seq++;
        GPacket p;
        buildHeader(p, type, target, 1, 0, len + ENC_OVERHEAD, flags,
                    sessionID, seq, DEFAULT_TTL);
        uint8_t blob[FRAME_PAYLOAD]; uint16_t blobLen = 0;
        if (encryptGCM_AAD(ps->shared_secret, p, data, len, blob, blobLen)) {
            memcpy(p.payload, blob, blobLen); p.payloadLen = blobLen;
            transmitFrame(p);
            return;
        }
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
// 25. HEARTBEAT (include x25519_pk)
// ============================================================================
static unsigned long lastHeartbeat = 0;
static void heartbeatTick() {
    if (nowMs() - lastHeartbeat < HEARTBEAT_INTERVAL_MS) return;
    lastHeartbeat = nowMs();
    x25519EnsureKeypair();

    outgoingStart(MSG_HEARTBEAT, BROADCAST_ID, g_x25519_pk, X25519_KEY_SIZE, FLAG_RELAY_OK);

    GPacket hb;
    buildHeader(hb, MSG_HEARTBEAT, BROADCAST_ID, 1, 0, X25519_KEY_SIZE,
                FLAG_RELAY_OK | FLAG_BROADCAST,
                ++g_sessionCounter, g_sessionCounter++, DEFAULT_TTL);
    memcpy(hb.payload, g_x25519_pk, X25519_KEY_SIZE);
    if (g_wifiReady) { hb.originProto = PROTO_WIFI; wifiSend(hb); }
    if (protoReady(PROTO_BT)) { hb.originProto = PROTO_BT; bleSend(hb); }
}

// ============================================================================
// 26. WEB UI
// ============================================================================
static bool webAuth(AsyncWebServerRequest* r) {
    if (!r->authenticate(WEB_USER, WEB_PASS)) { r->requestAuthentication(); return false; }
    return true;
}

static uint8_t parseHops(const String& s, uint32_t* out, uint8_t maxN) {
    uint8_t n = 0; int start = 0;
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

static String pageMain() {
    String h;
    h += "<!DOCTYPE HTML><html><head><meta charset='utf-8'>";
    h += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    h += "<title>Qorlyn</title><style>";
    h += "body{font-family:sans-serif;background:#181818;color:#eee;text-align:center;padding:12px;margin:0}";
    h += "button{padding:10px 14px;margin:4px;border:none;border-radius:6px;font-size:14px;cursor:pointer}";
    h += ".sos{background:#d32f2f;color:#fff}.msg{background:#388e3c;color:#fff}";
    h += ".key{background:#fbc02d;color:#000}.onion{background:#5e35b1;color:#fff}";
    h += ".ok{background:#2e7d32;color:#fff}.warn{background:#ef6c00;color:#fff}.danger{background:#c62828;color:#fff}";
    h += "input[type=text]{padding:10px;width:85%;margin:6px;border-radius:6px;border:none;max-width:520px}";
    h += ".hint{font-size:12px;color:#888;margin-top:4px}";
    h += ".card{background:#222;border-radius:8px;padding:12px;margin:12px auto;max-width:640px;text-align:left}";
    h += ".row{display:flex;justify-content:space-between;align-items:center;border-bottom:1px solid #333;padding:8px 0;gap:8px;flex-wrap:wrap}";
    h += ".mono{font-family:ui-monospace,monospace;font-size:13px}";
    h += "</style></head><body>";

    char idbuf[16]; sprintf(idbuf, "%08X", myID);
    h += "<h2>Qorlyn</h2><div class='mono'>ID: "; h += idbuf; h += "</div>";
    h += "<div class='hint'>LoRa:"; h += g_loraReady ? "ON" : "off";
    h += " | WiFi:"; h += g_wifiReady ? (WiFi.status()==WL_CONNECTED ? "STA+AP" : "AP") : "off";
    h += " | BLE:";  h += (g_btReady && g_bleCentralConnected) ? "conn" : (g_btReady ? "adv" : "off");
    h += " | Policy: "; h += policyName(g_policy); h += "</div>";

    h += "<div class='card'><h3>PENDING</h3>";
    bool any = false;
    for (int i = 0; i < MAX_PEERS; i++) {
        if (peers[i].state == PEER_PENDING_IN) {
            any = true;
            char b[16]; sprintf(b, "%08X", peers[i].peerID);
            h += "<div class='row'><span class='mono'>"; h += b; h += " pending-in</span><span>";
            h += "<button class='ok' onclick=\"a('/approve?id="; h += b; h += "')\">accetta</button>";
            h += "<button class='warn' onclick=\"a('/reject?id="; h += b; h += "')\">rifiuta</button>";
            h += "<button class='danger' onclick=\"a('/block?id="; h += b; h += "')\">blocca</button>";
            h += "</span></div>";
        }
        if (peers[i].state == PEER_PENDING_OUT) {
            any = true;
            char b[16]; sprintf(b, "%08X", peers[i].peerID);
            h += "<div class='row'><span class='mono'>"; h += b; h += " pending-out</span>";
            h += "<span><button class='warn' onclick=\"a('/forget?id="; h += b; h += "')\">annulla</button></span></div>";
        }
    }
    if (!any) h += "<div class='hint'>nessuna</div>";
    h += "</div>";

    h += "<div class='card'><h3>ESTABLISHED</h3>";
    any = false;
    for (int i = 0; i < MAX_PEERS; i++) {
        if (peers[i].state != PEER_ESTABLISHED) continue;
        any = true;
        char b[16]; sprintf(b, "%08X", peers[i].peerID);
        h += "<div class='row'><span class='mono'>"; h += b;
        h += " ("; h += protoName(peers[i].lastSeenProto); h += ")";
        h += peers[i].has_x25519_pk ? " x25519:ok" : " x25519:no";
        h += "</span><span>";
        h += "<button class='warn' onclick=\"a('/forget?id="; h += b; h += "')\">dimentica</button>";
        h += "<button class='danger' onclick=\"a('/block?id="; h += b; h += "')\">blocca</button>";
        h += "</span></div>";
    }
    if (!any) h += "<div class='hint'>nessuna</div>";
    h += "</div>";

    h += "<div class='card'><h3>BLOCKED</h3>";
    for (uint8_t i = 0; i < g_blocklist_count; i++) {
        char b[16]; sprintf(b, "%08X", g_blocklist[i]);
        h += "<div class='row'><span class='mono'>"; h += b;
        h += "</span><button class='ok' onclick=\"a('/unblock?id="; h += b; h += "')\">sblocca</button></div>";
    }
    if (!g_blocklist_count) h += "<div class='hint'>nessuno</div>";
    h += "</div>";

    h += "<div class='card'><h3>POLICY</h3>";
    const char* pols[] = {"auto","manual","whitelist","hybrid"};
    for (int i = 0; i < 4; i++) {
        h += "<button class='key' onclick=\"a('/policy?p="; h += String(i); h += "')\">";
        h += pols[i]; if (g_policy == (uint8_t)i) h += " *";
        h += "</button>";
    }
    h += "</div>";

    h += "<div class='card'><h3>WHITELIST</h3>";
    for (uint8_t i = 0; i < g_whitelist_count; i++) {
        char b[16]; sprintf(b, "%08X", g_whitelist[i]);
        h += "<div class='row'><span class='mono'>"; h += b;
        h += "</span><button class='danger' onclick=\"a('/wl_del?id="; h += b; h += "')\">rimuovi</button></div>";
    }
    h += "<div class='row'><input type='text' id='wlid' placeholder='ID hex' style='width:auto;flex:1'>";
    h += "<button class='msg' onclick=\"wl()\">aggiungi</button></div></div>";

    h += "<div class='card'><h3>INVIA</h3>";
    h += "<input type='text' id='t' placeholder='Target ID' value='FFFFFFFF'>";
    h += "<input type='text' id='hops' placeholder='Onion hops (hex comma sep.)' value=''>";
    h += "<input type='text' id='m' placeholder='Messaggio'>";
    h += "<label><input type='checkbox' id='ack' checked> ACK end-to-end</label>";
    h += "<br><button class='key' onclick=\"s('KEY')\">HANDSHAKE</button>";
    h += "<button class='msg' onclick=\"s('CHAT')\">CHAT</button>";
    h += "<button class='sos' onclick=\"s('SOS')\">SOS</button>";
    h += "<button class='onion' onclick=\"s('ONION')\">ONION</button>";
    h += "</div>";

    h += "<script>";
    h += "function a(u){fetch(u).then(r=>r.ok?location.reload():r.text().then(t=>alert(t)));}";
    h += "function wl(){var v=document.getElementById('wlid').value;if(v)a('/wl_add?id='+v);}";
    h += "function s(t){";
    h += "var tg=document.getElementById('t').value,hp=document.getElementById('hops').value,";
    h += "msg=document.getElementById('m').value,ack=document.getElementById('ack').checked?'1':'0';";
    h += "send(0);function send(f){fetch('/send',{method:'POST',";
    h += "headers:{'Content-Type':'application/x-www-form-urlencoded'},";
    h += "body:'type='+t+'&target='+tg+'&hops='+hp+'&ack='+ack+'&msg='+encodeURIComponent(msg)+'&force='+f";
    h += "}).then(r=>{if(r.status===409){if(confirm('Target senza sessione. In chiaro?'))send(1);}";
    h += "else if(!r.ok)r.text().then(x=>alert(x));});}}";
    h += "</script></body></html>";
    return h;
}

static void setupWeb() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!webAuth(req)) return;
        lastActivity = nowMs();
        req->send(200, "text/html", pageMain());
    });
    server.on("/approve", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!webAuth(req)) return;
        if (!req->hasParam("id")) { req->send(400,"text/plain","no id"); return; }
        uint32_t id = strtoul(req->getParam("id")->value().c_str(), nullptr, 16);
        bool ok = peerAcceptPending(id);
        req->send(ok?200:400, "text/plain", ok?"ok":"no");
    });
    server.on("/reject", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!webAuth(req)) return;
        if (!req->hasParam("id")) { req->send(400,"text/plain","no id"); return; }
        uint32_t id = strtoul(req->getParam("id")->value().c_str(), nullptr, 16);
        bool ok = peerRejectPending(id);
        req->send(ok?200:400, "text/plain", ok?"ok":"no");
    });
    server.on("/forget", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!webAuth(req)) return;
        if (!req->hasParam("id")) { req->send(400,"text/plain","no id"); return; }
        peerForget(strtoul(req->getParam("id")->value().c_str(), nullptr, 16));
        req->send(200, "text/plain", "ok");
    });
    server.on("/block", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!webAuth(req)) return;
        if (!req->hasParam("id")) { req->send(400,"text/plain","no id"); return; }
        uint32_t id = strtoul(req->getParam("id")->value().c_str(), nullptr, 16);
        peerForget(id); addBlocklist(id); removeWhitelist(id);
        req->send(200, "text/plain", "ok");
    });
    server.on("/unblock", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!webAuth(req)) return;
        if (!req->hasParam("id")) { req->send(400,"text/plain","no id"); return; }
        removeBlocklist(strtoul(req->getParam("id")->value().c_str(), nullptr, 16));
        req->send(200, "text/plain", "ok");
    });
    server.on("/policy", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!webAuth(req)) return;
        if (!req->hasParam("p")) { req->send(400,"text/plain","no p"); return; }
        int v = atoi(req->getParam("p")->value().c_str());
        if (v < 0 || v > 3) { req->send(400,"text/plain","bad"); return; }
        g_policy = (uint8_t)v;
        req->send(200, "text/plain", "ok");
    });
    server.on("/wl_add", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!webAuth(req)) return;
        if (!req->hasParam("id")) { req->send(400,"text/plain","no id"); return; }
        uint32_t id = strtoul(req->getParam("id")->value().c_str(), nullptr, 16);
        if (isBlocked(id)) { req->send(400,"text/plain","blocked"); return; }
        addWhitelist(id);
        req->send(200, "text/plain", "ok");
    });
    server.on("/wl_del", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (!webAuth(req)) return;
        if (!req->hasParam("id")) { req->send(400,"text/plain","no id"); return; }
        removeWhitelist(strtoul(req->getParam("id")->value().c_str(), nullptr, 16));
        req->send(200, "text/plain", "ok");
    });
    server.on("/send", HTTP_POST, [](AsyncWebServerRequest* req) {
        if (!webAuth(req)) return;
        lastActivity = nowMs();
        if (!req->hasParam("type", true) || !req->hasParam("target", true)) {
            req->send(400,"text/plain","param"); return;
        }
        String type = req->getParam("type", true)->value();
        String tstr = req->getParam("target", true)->value();
        String hopsStr = req->hasParam("hops", true) ? req->getParam("hops", true)->value() : "";
        String msg  = req->hasParam("msg", true) ? req->getParam("msg", true)->value() : "";
        bool ackReq = req->hasParam("ack", true) && req->getParam("ack", true)->value() == "1";
        bool force  = req->hasParam("force", true) && req->getParam("force", true)->value() == "1";

        char* endp = nullptr;
        uint32_t target = strtoul(tstr.c_str(), &endp, 16);
        if (!endp || *endp != '\0' || tstr.length() == 0) {
            req->send(400,"text/plain","target"); return;
        }
        if (isBlocked(target)) { req->send(400,"text/plain","blocked"); return; }

        if (type == "KEY") {
            bool ok = initiateHandshake(target);
            req->send(ok?200:400, "text/plain", ok?"ok":"fail");
            return;
        }
        if (type == "ONION") {
            uint32_t hops[ONION_MAX_HOPS - 1];
            uint8_t nHops = parseHops(hopsStr, hops, ONION_MAX_HOPS - 1);
            if (msg.length() > MAX_MESSAGE_BYTES) { req->send(413,"text/plain","msg long"); return; }
            bool ok = onionSend(MSG_CHAT, target,
                                (const uint8_t*)msg.c_str(), msg.length(),
                                hops, nHops, ackReq);
            req->send(ok?200:400, "text/plain", ok?"ok":"onion fail");
            return;
        }
        if ((type == "SOS" || type == "CHAT") && target != BROADCAST_ID) {
            PeerSession* ps = peerFind(target);
            bool willEncrypt = (ps && ps->state == PEER_ESTABLISHED);
            if (type == "CHAT" && msg.length() + ENC_OVERHEAD > FRAME_PAYLOAD) willEncrypt = false;
            if (!willEncrypt && !force) {
                req->send(409, "text/plain", "PLAINTEXT_FALLBACK"); return;
            }
        }
        if (type == "SOS") {
            static const uint8_t empty = 0;
            sendSecureOrPlain(MSG_SOS_MEDIC, target, &empty, 0);
        }
        else if (type == "CHAT") {
            if (msg.length() > MAX_MESSAGE_BYTES) { req->send(413,"text/plain","msg long"); return; }
            sendSecureOrPlain(MSG_CHAT, target, (const uint8_t*)msg.c_str(), msg.length());
        } else {
            req->send(400,"text/plain","tipo"); return;
        }
        req->send(200, "text/plain", "ok");
    });
    server.begin();
}

// ============================================================================
// 27. SERIAL
// ============================================================================
static char   s_cmdBuf[192];
static size_t s_cmdLen = 0;

static bool parseHex32(const char* s, uint32_t* out) {
    if (!s || !*s) return false;
    char* endp = nullptr;
    uint32_t v = strtoul(s, &endp, 16);
    if (endp == s || *endp != 0) return false;
    *out = v;
    return true;
}

static void serialHelp() {
    Serial.println(F("--- Qorlyn serial commands ---"));
    Serial.println(F("H <id>            handshake verso id"));
    Serial.println(F("a / r             accept / reject primo pending"));
    Serial.println(F("A                 lista pending-in"));
    Serial.println(F("C <id> <msg>      chat cifrata (o plaintext se no sessione)"));
    Serial.println(F("S <id>            SOS medico"));
    Serial.println(F("B <msg>           broadcast chat"));
    Serial.println(F("O <id> <h1,h2|-> <msg>  onion"));
    Serial.println(F("M                 toggle policy"));
    Serial.println(F("p                 lista peer"));
    Serial.println(F("P                 mostra policy"));
    Serial.println(F("W                 stato trasporti"));
    Serial.println(F("i                 info interferenza"));
    Serial.println(F("F <id>            forget peer"));
    Serial.println(F("X <id>            block peer"));
    Serial.println(F("Z                 azzera peers e pending"));
    Serial.println(F("Y                 toggle BLE central scan"));
    Serial.println(F("R                 re-init radio LoRa"));
    Serial.println(F("h                 hello broadcast"));
    Serial.println(F("k / x             kyber / x25519 keypair"));
    Serial.println(F("?                 mostra questo aiuto"));
}

static void serialProcessLine(const char* line) {
    while (*line == ' ' || *line == '\t') line++;
    if (!*line) return;
    char cmd = line[0];
    const char* a1 = line + 1;
    while (*a1 == ' ' || *a1 == '\t') a1++;

    switch (cmd) {
        case 'H': {
            uint32_t id;
            if (!parseHex32(a1, &id)) { Serial.println(F("uso: H <id_hex>")); return; }
            initiateHandshake(id);
            break;
        }
        case 'a': {
            bool any = false;
            for (int i = 0; i < MAX_PEERS; i++)
                if (peers[i].state == PEER_PENDING_IN) { peerAcceptPending(peers[i].peerID); any = true; break; }
            if (!any) Serial.println(F("nessun pending da accettare"));
            break;
        }
        case 'r': {
            bool any = false;
            for (int i = 0; i < MAX_PEERS; i++)
                if (peers[i].state == PEER_PENDING_IN) { peerRejectPending(peers[i].peerID); any = true; break; }
            if (!any) Serial.println(F("nessun pending da rifiutare"));
            break;
        }
        case 'A': {
            int idx = 0;
            for (int i = 0; i < MAX_PEERS; i++)
                if (peers[i].state == PEER_PENDING_IN)
                    Serial.printf("[%d] pending-in %08X\n", idx++, peers[i].peerID);
            if (!idx) Serial.println(F("nessun pending"));
            break;
        }
        case 'C': {
            char buf[192];
            strncpy(buf, a1, sizeof(buf) - 1); buf[sizeof(buf) - 1] = 0;
            char* sp = strchr(buf, ' ');
            if (!sp) { Serial.println(F("uso: C <id_hex> <msg>")); return; }
            *sp = 0;
            uint32_t id;
            if (!parseHex32(buf, &id)) { Serial.println(F("bad id")); return; }
            const char* msg = sp + 1;
            sendSecureOrPlain(MSG_CHAT, id, (const uint8_t*)msg, strlen(msg));
            break;
        }
        case 'S': {
            uint32_t id;
            if (!parseHex32(a1, &id)) { Serial.println(F("uso: S <id_hex>")); return; }
            static const uint8_t empty = 0;
            sendSecureOrPlain(MSG_SOS_MEDIC, id, &empty, 0);
            break;
        }
        case 'B': {
            sendSecureOrPlain(MSG_CHAT, BROADCAST_ID, (const uint8_t*)a1, strlen(a1));
            break;
        }
        case 'O': {
            char buf[256];
            strncpy(buf, a1, sizeof(buf) - 1); buf[sizeof(buf) - 1] = 0;
            char* p1 = strchr(buf, ' ');
            if (!p1) { Serial.println(F("uso: O <id_hex> <h1,h2|-> <msg>")); return; }
            *p1 = 0;
            char* p2 = strchr(p1 + 1, ' ');
            if (!p2) { Serial.println(F("uso: O <id_hex> <h1,h2|-> <msg>")); return; }
            *p2 = 0;
            uint32_t target;
            if (!parseHex32(buf, &target)) { Serial.println(F("bad target")); return; }
            uint32_t hops[ONION_MAX_HOPS - 1];
            uint8_t nHops = 0;
            if (*(p1 + 1) && strcmp(p1 + 1, "-") != 0) {
                char* save = nullptr;
                char* tok = strtok_r(p1 + 1, ",", &save);
                while (tok && nHops < ONION_MAX_HOPS - 1) {
                    uint32_t v = strtoul(tok, nullptr, 16);
                    if (v) hops[nHops++] = v;
                    tok = strtok_r(nullptr, ",", &save);
                }
            }
            const char* msg = p2 + 1;
            bool ok = onionSend(MSG_CHAT, target, (const uint8_t*)msg, strlen(msg), hops, nHops, true);
            Serial.printf("[ONION] %s\n", ok ? "queued" : "FAIL");
            break;
        }
        case 'M': {
            g_policy = (g_policy + 1) & 0x03;
            Serial.printf("policy=%s\n", policyName(g_policy));
            break;
        }
        case 'F': {
            uint32_t id;
            if (!parseHex32(a1, &id)) { Serial.println(F("uso: F <id_hex>")); return; }
            peerForget(id);
            Serial.printf("forget %08X\n", id);
            break;
        }
        case 'X': {
            uint32_t id;
            if (!parseHex32(a1, &id)) { Serial.println(F("uso: X <id_hex>")); return; }
            peerForget(id);
            addBlocklist(id);
            Serial.printf("blocked %08X\n", id);
            break;
        }
        case 'Z': {
            memset(peers, 0, sizeof(peers));
            memset(g_pending, 0, sizeof(g_pending));
            Serial.println(F("peers e pending azzerati"));
            break;
        }
        case 'Y': {
            g_bleScanEnabled = !g_bleScanEnabled;
            Serial.printf("BLE central scan %s\n", g_bleScanEnabled ? "ON" : "OFF");
            if (!g_bleScanEnabled) {
                for (int i = 0; i < BLE_MAX_CENTRALS; i++) {
                    if (g_bleCentrals[i].active) {
                        if (g_bleCentrals[i].client) g_bleCentrals[i].client->disconnect();
                        delete g_bleCentrals[i].client;
                        memset(&g_bleCentrals[i], 0, sizeof(BleCentralLink));
                    }
                }
            }
            break;
        }
        case 'R': {
            Serial.println(F("[RADIO] re-init"));
            radio.standby();
            delay(50);
            radio.startReceive();
            Serial.println(F("[RADIO] ok"));
            break;
        }
        case 'p': {
            int n = 0;
            for (int i = 0; i < MAX_PEERS; i++) {
                if (peers[i].state == PEER_NONE) continue;
                Serial.printf("peer %08X [%s] via=%s x25519=%d\n",
                              peers[i].peerID, peerStateName(peers[i].state),
                              protoName(peers[i].lastSeenProto),
                              peers[i].has_x25519_pk);
                n++;
            }
            if (n == 0) Serial.println(F("nessun peer"));
            break;
        }
        case 'P': {
            Serial.printf("policy=%s wl=%u bl=%u\n",
                          policyName(g_policy), g_whitelist_count, g_blocklist_count);
            break;
        }
        case 'W': {
            Serial.printf("LoRa:%d WiFi:%d BT:%d bleConn:%d centrals:%d/%d scan:%d\n",
                          g_loraReady, g_wifiReady, g_btReady, g_bleCentralConnected,
                          g_bleCentrals[0].active + g_bleCentrals[1].active,
                          BLE_MAX_CENTRALS, g_bleScanEnabled);
            Serial.printf("WiFi STA:%s AP:%s\n",
                          WiFi.status()==WL_CONNECTED ? WiFi.localIP().toString().c_str() : "no",
                          (WiFi.getMode() & WIFI_MODE_AP) ? WiFi.softAPIP().toString().c_str() : "no");
            break;
        }
        case 'i': {
            Serial.printf("FEC=%u INT=%u foreign=%lu crc=%lu\n",
                          DynamicFEC(interference_level), interference_level,
                          (unsigned long)foreign_packets_count,
                          (unsigned long)crc_error_count);
            break;
        }
        case 'h': {
            const char* m = "Hello world";
            sendSecureOrPlain(MSG_CHAT, BROADCAST_ID, (const uint8_t*)m, strlen(m));
            break;
        }
        case 'k': kyberEnsureKeypair(); Serial.println(F("[KYBER] PK ready")); break;
        case 'x': x25519EnsureKeypair(); Serial.println(F("[X25519] PK ready")); break;
        case '?': serialHelp(); break;
        default:
            Serial.printf("cmd '%c' sconosciuto. usa '?'\n", cmd);
            break;
    }
}

static void serialTick() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (s_cmdLen > 0) {
                s_cmdBuf[s_cmdLen] = 0;
                serialProcessLine(s_cmdBuf);
                s_cmdLen = 0;
            }
        } else if (s_cmdLen < sizeof(s_cmdBuf) - 1) {
            s_cmdBuf[s_cmdLen++] = c;
        }
    }
}

// ============================================================================
// 28. SLEEP
// ============================================================================
static void enterSleep() {
    updateUI("SLEEP", "bye");
    delay(200);
    oled.ssd1306_command(SSD1306_DISPLAYOFF);
    server.end();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    BLEDevice::deinit(true);
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
// 29. SETUP
// ============================================================================
static bool setupRadio() {
    Serial.println("[RADIO] init");
    spi.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
    pinMode(LORA_CS, OUTPUT);  digitalWrite(LORA_CS, HIGH);
    pinMode(LORA_RST, OUTPUT); digitalWrite(LORA_RST, LOW); delay(100);
                               digitalWrite(LORA_RST, HIGH); delay(200);
    pinMode(LORA_BUSY, INPUT);
    delay(50);
    int st = radio.begin(868.0, 125.0, 9, DynamicFEC(interference_level), 0x12, 10, 8);
    if (st != RADIOLIB_ERR_NONE) { Serial.printf("[RADIO] err=%d\n", st); return false; }
    radio.setCRC(true);
    radio.setOutputPower(10);
    delay(50);
    radio.startReceive();
    g_loraReady = true;
    Serial.println("[RADIO] ready");
    return true;
}

static void setupOLED() {
    pinMode(OLED_PWR, OUTPUT); digitalWrite(OLED_PWR, LOW); delay(300);
    pinMode(OLED_RST, OUTPUT); digitalWrite(OLED_RST, LOW); delay(100);
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
// 30. MAIN TASK (sostituisce loop() per avere 16 KB di stack)
// ============================================================================
static void mainTask(void*) {
    for (;;) {
        if (nowMs() - lastActivity > 10UL * 60UL * 1000UL) enterSleep();

        // Processa pacchetti BLE ricevuti (dequeued dalle callback NimBLE)
        GPacket bp;
        while (xQueueReceive(bleRxQueue, &bp, 0) == pdTRUE) {
            if (xSemaphoreTakeRecursive(stateMutex, portMAX_DELAY) == pdTRUE) {
                handleReceivedPacketFromProto(bp, PROTO_BT);
                xSemaphoreGiveRecursive(stateMutex);
            }
        }

        if (xSemaphoreTakeRecursive(stateMutex, portMAX_DELAY) == pdTRUE) {
            outgoingTick();
            rxSlotsSweep();
            peersSweep();
            onionTrackerSweep();
            heartbeatTick();
            serialTick();
            xSemaphoreGiveRecursive(stateMutex);
        }
        wifiDataTick();
        // wifiStaTry();  // DISABILITATO: stack overflow

        if (nowMs() < g_plaintextWarnUntil) updateUI("PLAINTEXT!", "msg in chiaro");
        else {
            int pend = 0;
            for (int i = 0; i < MAX_PEERS; i++) if (peers[i].state == PEER_PENDING_IN) pend++;
            char b[24];
            if (pend) { snprintf(b, sizeof(b), "%d richieste", pend); updateUI("PENDING", b); }
            else { snprintf(b, sizeof(b), "via %s", protoName(g_lastRxProto)); updateUI("READY", b); }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("\n=== Qorlyn ===");
    Serial.printf("GPacket=%u Peer=%u\n", (unsigned)sizeof(GPacket), (unsigned)sizeof(PeerSession));

    // Distinzione cold boot vs warm reset via RTC magic
    if (g_rtc_magic != RTC_MAGIC) {
        Serial.println("[BOOT] cold boot: reset completo RTC");
        memset(peers, 0, sizeof(peers));
        g_sessionCounter = 0;
        g_policy = POLICY_MANUAL;
        g_blocklist_count = 0;
        g_whitelist_count = 0;
        g_kyber_ready = false;
        g_x25519_ready = false;
        g_rtc_magic = RTC_MAGIC;
    } else {
        Serial.println("[BOOT] warm reset: stato RTC preservato");
    }

    myID = g_myID ? g_myID : generateDeviceID();
    g_myID = myID;
    Serial.printf("myID=%08X policy=%s\n", myID, policyName(g_policy));

    stateMutex = xSemaphoreCreateRecursiveMutex();
    txQueue    = xQueueCreate(16, sizeof(TxRequest));
    bleRxQueue = xQueueCreate(8, sizeof(GPacket));

    memset(g_pending, 0, sizeof(g_pending));
    memset(rxSlots, 0, sizeof(rxSlots));
    memset(outgoing, 0, sizeof(outgoing));
    memset(g_onionTrackers, 0, sizeof(g_onionTrackers));
    memset(g_bleCentrals, 0, sizeof(g_bleCentrals));

    setupOLED();
    printWakeReason();
    if (!setupRadio()) updateUI("ERROR", "radio fail");

    xTaskCreatePinnedToCore(radioTask, "radio", 8192, nullptr, 2, &radioTaskHandle, 1);
    xTaskCreatePinnedToCore(mainTask, "main", 16384, nullptr, 2, nullptr, 1);

    if (!g_kyber_ready) kyberEnsureKeypair();
    x25519EnsureKeypair();

    wifiSetupSync();
    setupWeb();
    bleSetup();

    lastActivity = nowMs();
    updateUI("READY", policyName(g_policy));
    Serial.println("[BOOT] pronto. digita '?' per i comandi");
}

void loop() {
    // Tutto il lavoro gira su mainTask (stack 16 KB)
    // loopTask di Arduino resta solo per compatibilita'
    vTaskDelay(pdMS_TO_TICKS(1000));
}

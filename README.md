<!--
  ═══════════════════════════════════════════════════════════════
  QORLYN — Quantum Onion Routing Lattice lYnx Network
  README ufficiale · v0.1-alpha
  ═══════════════════════════════════════════════════════════════
-->

<div align="center">

<!-- LOGO — sostituire con il file reale quando pronto -->
<!-- <img src="assets/qorlyn-logo.svg" alt="Qorlyn" width="180"> -->

# QORLYN

### Riconnettiamo i puntini obbligati al silenzio.

**Quantum Onion Routing Lattice lYnx Network**

Comunicazione mesh decentralizzata, post-quantistica, resistente alla censura.
Nata sul campo. Per chi non ha alternative.

<br>

<!-- ═══════════ SHIELDS ═══════════ -->

[![License](https://img.shields.io/badge/LICENSE-GPLv3-00B8D4?style=for-the-badge&logo=gnu&logoColor=white)](LICENSE)
[![Status](https://img.shields.io/badge/STATUS-ALPHA-E8A800?style=for-the-badge)]()
[![Version](https://img.shields.io/badge/VERSION-0.1.0--alpha-0A0A0A?style=for-the-badge)]()
[![Platform](https://img.shields.io/badge/PLATFORM-ESP32--S3-2A7FA8?style=for-the-badge&logo=espressif&logoColor=white)]()
[![Radio](https://img.shields.io/badge/RADIO-LoRa%20868%20MHz-5C8B2F?style=for-the-badge)]()
[![PQC](https://img.shields.io/badge/PQC-KYBER--512-00B8D4?style=for-the-badge)]()

[![Onion](https://img.shields.io/badge/ONION-3%20HOP-0F3A52?style=for-the-badge)]()
[![Server](https://img.shields.io/badge/SERVER-0-0A0A0A?style=for-the-badge)]()
[![No Profit](https://img.shields.io/badge/NON--PROFIT-100%25-5C8B2F?style=for-the-badge)]()
[![PRs](https://img.shields.io/badge/PRs-WELCOME-00B8D4?style=for-the-badge)]()
[![Made in Italy](https://img.shields.io/badge/MADE%20IN-ITALY-0A0A0A?style=for-the-badge&logo=italy&logoColor=white)]()

<br>

[**Cos'è**](#-cosè-qorlyn) · [**Filosofia**](#-filosofia) · [**Casi d'uso**](#-casi-duso) · [**Hardware**](#-hardware-lynx) · [**Installazione**](#-installazione) · [**Supporto**](#-supporto) · [**Contatti**](#-contatti)

</div>

---

## 🛰 Cos'è Qorlyn

**Qorlyn** è un protocollo di comunicazione **decentralizzato, anonimo e post-quantistico**, progettato per funzionare **dove tutto il resto si spegne**.

Non è un prodotto. È uno **standard aperto**. Non dipende da server, operatori, satelliti o infrastrutture centralizzate. Ogni dispositivo è un nodo. Ogni nodo è pari. La rete si forma dove serve, si scioglie quando non serve.

Qorlyn nasce da un'esigenza concreta: **consentire a giornalisti, attivisti, operatori umanitari e civili di comunicare in sicurezza in zone di conflitto, sotto censura, o in aree senza infrastruttura.**

> **Qorlyn è la rete. Lynx è il dispositivo. Il protocollo è di tutti.**

---

## 🧭 Filosofia

Viviamo nel futuro che ci era stato promesso. Solo che il futuro promesso era **luminoso, aperto, libero**. Quello che abbiamo è **sorvegliato, centralizzato, silenzioso**.

La censura nel 2026 è **antiquata**. Eppure esiste.
La sorveglianza di massa è **inutile**. Eppure esiste.
La dipendenza da infrastrutture centralizzate è **fragile**. Eppure esiste.

**Qorlyn è la risposta tecnica a un problema umano.**

Non è nostalgia. È **memoria armata**. Ricordiamo ciò che doveva essere, e costruiamo ciò che deve essere.

> *Il futuro non è vuoto. È pieno di informazione. La differenza è la chiarezza.*

---

## 💎 Valori

Qorlyn non è neutrale. Ha dei valori precisi, non negoziabili.

| Valore | Cosa significa |
|---|---|
| **Aperto** | Codice, hardware, protocollo: tutto è pubblico e verificabile. |
| **No-profit** | Non può essere comprato, non può essere venduto. Solo diffuso. |
| **Anonimo** | Nessuna identità richiesta. Nessuna traccia lasciata. |
| **Resiliente** | Funziona senza infrastruttura, senza permessi, senza autorità. |
| **Trasparente** | Sicurezza tramite trasparenza, non tramite segreto aziendale. |
| **Umano** | Progettato per chi rischia la vita per comunicare. |

---

## 🎯 Casi d'uso

Qorlyn non è per tutti. **È per chi non ha alternative.**

### 📰 Giornalisti e reporter indipendenti
Comunicare da zone di conflitto, sotto censura, o in aree senza copertura. Proteggere le fonti. Trasmettere materiale senza tracciabilità.

### ✊ Attivisti e difensori dei diritti umani
Coordinarsi durante proteste, in contesti autoritari, o in aree con blackout informativo. Nessuna SIM, nessun IMEI, nessun registro.

### 🚑 ONG e operatori umanitari
Coordinare squadre in campi profughi, zone di disastro, o aree remote. Comunicazioni locali a costo quasi zero, complementari alle costose radio satellitari.

### 🛡 Protezione civile e comuni
Gestire emergenze quando le reti cellulari ed elettriche collassano per disastri naturali. Kit pre-configurati, dispiegabili in meno di 5 minuti.

### 🏛 Enti governativi e difesa
Continuità operativa in scenari di cyber-warfare. Comunicazioni tattiche senza infrastruttura fissa, resistenti a jamming e triangolazione.

### 👥 Comunità civili
Resilienza civica. Chiunque possieda un dispositivo Qorlyn può unirsi alla rete, creando una maglia di comunicazione libera e indipendente.

---

## ⚙ Architettura

Qorlyn si fonda su tre pilastri tecnici integrati.

### 1. Crittografia Post-Quantistica (PQC)
Scambio di chiavi basato su **reticoli matematici** (Lattice-Based Cryptography), resistente alla potenza di calcolo dei futuri computer quantistici.

- **Algoritmo:** Kyber-512 (ML-KEM)
- **Cifratura simmetrica:** AES-256-GCM con AAD
- **Chiavi effimere:** rigenerate ad ogni sessione
- **PUF:** chiavi derivate dalle imperfezioni del silicio (uniche per dispositivo)

### 2. Onion Routing Dinamico
Ogni pacchetto attraversa **tre nodi casuali** prima di raggiungere la destinazione. Ogni hop conosce solo il nodo precedente e quello successivo.

- **3 hop** per default
- **Cifratura a strati** (il pacchetto è sigillato 3 volte)
- **Nessun nodo** vede insieme mittente e destinatario
- **Anti-triangolazione** per posizione fisica

### 3. Trasporto Fisico LoRa
Modulazione **Chirp Spread Spectrum** su banda ISM libera.

- **868 MHz** (EU) / 915 MHz (US) / 433 MHz (globale)
- **Fino a 12 km** in campo aperto con 1.5 W
- **Sotto la soglia del rumore** ambientale
- **Resistente a jamming** per natura della modulazione

---

## 📟 Hardware: Lynx

**Lynx** è il dispositivo portatile per l'utente finale. Un computer di rete a basso consumo, robusto, pensato per essere usato dove nessun altro dispositivo sopravvive.

### Specifiche

| Componente | Dettaglio |
|---|---|
| **MCU** | ESP32-S3 |
| **Radio** | SX1262 (LoRa) |
| **Display** | OLED 128×64 |
| **Crittografia** | Kyber-512 + AES-256-GCM |
| **Chassis** | Shockproof IK09 |
| **Ambiente** | IP67 Water / Dust |
| **Temperature** | −20°C / +60°C |
| **Alimentazione** | Doppia batteria (interna + intercambiabile) |
| **Ricarica** | Pannello solare 20 W + USB-C |
| **Sensori** | Temperatura, gas, fumo, radiazioni |
| **Sicurezza hardware** | PUF, Secure Boot, Wipe fisico |
| **Connettività** | LoRa mesh + gateway WiFi/BT/Internet |

### Sicurezza hardware

- **PUF:** le chiavi Kyber non sono memorizzate, ma derivate dalle imperfezioni fisiche del silicio. Uniche per dispositivo. Non estraibili.
- **Secure Boot:** catena di fiducia che parte dal ROM. Nessun firmware non firmato può essere eseguito.
- **Wipe fisico:** un gesto meccanico distrugge fisicamente il chip, la memoria e ogni traccia. Non solo le chiavi. **Tutto.**
- **GPS e microfono rimovibili:** fisicamente staccabili per privacy totale.

### Versioni

- **Lynx Tactical** — Chassis nero opaco, IP67, display ambra, uso militare e difesa.
- **Lynx Aero** — Chassis traslucido, LED aqua, uso civile, giornalistico, appassionati.

*Stesso firmware. Stesso protocollo. Due anime.*

---

## 🚀 Installazione

### Requisiti

- 2× Heltec V3.2 (o board compatibili ESP32 + SX1262)
- Arduino IDE 2.x o PlatformIO
- RadioLib ≥ 6.0
- mbedtls (incluso nell'ESP32 core)
- Libreria `pqc_kyber` ([DA COMPLETARE: link])

### Build

```bash
git clone https://github.com/qorlyn/firmware.git
cd firmware
pio run -e heltec_v3
pio run -e heltec_v3 -t upload


# Qorlyn Project

**Firmware ESP32 per comunicazioni mesh LoRa con crittografia post-quantum**

---

## 1. Panoramica

Qorlyn Project è un firmware per ESP32 che implementa un sistema di comunicazione mesh su LoRa con:

- **Scambio di chiavi post-quantum** basato su **ML-KEM (Kyber-512)**, tramite una versione adattata di [PQClean](https://github.com/PQClean/PQClean).
- **Cifratura simmetrica AES-256-GCM** con AAD (Additional Authenticated Data).
- **Trasporto affidabile** con frammentazione, riassemblaggio, ACK bitmap e retry.
- **Mesh relay** con TTL e deduplicazione.
- **Web UI** in modalità Access Point per l'invio di messaggi.
- **Display OLED** per stato e messaggi.
- **Deep sleep** con wake-up su pulsante, radio o timer.

Il progetto è pensato per scenari off-grid, emergenze e comunicazioni tattiche dove la riservatezza e la resilienza della rete sono requisiti primari.

---

## 2. Caratteristiche principali

| Funzionalità | Dettaglio |
|---|---|
| KEM post-quantum | ML-KEM / Kyber-512 (PK 800 B, SK 1632 B, CT 768 B) |
| Cifratura simmetrica | AES-256-GCM con AAD, IV 12 B, tag 16 B |
| Overhead cifratura | 28 byte per frame |
| Frammentazione | Fino a 8 frame da 128 B → 1024 B per messaggio |
| Affidabilità | ACK bitmap, 3 retry, timeout 3 s, dedup cache 32 voci |
| Mesh | TTL default 3, relay se `FLAG_RELAY_OK` e TTL > 0 |
| Web UI | AP `G-TALK-XXXXXXXX`, HTTP Basic Auth |
| OLED | SSD1306 128×64 I2C |
| Radio | SX1262 868 MHz, SF9, BW125, sync word 0x12 |
| Power management | Deep sleep dopo 10 min di inattività |
| Persistenza RTC | ID, session counter, chiavi Kyber |

---

## 3. Hardware richiesto

- **ESP32** (con supporto FSPI)
- **Modulo LoRa SX1262** (es. Ra-01SH, E22-900M22S)
- **Display OLED SSD1306** 128×64 I2C
- **Pulsante** su GPIO0 per wake-up
- Alimentazione 3.3 V
O un Heltec Meshtastic LoRa V3.2, che combina Modulo LoRa SX1262, Display OLED SSD1306.

### Pinout

| Periferica | Segnale | GPIO |
|---|---|---|
| OLED | SDA | 17 |
| OLED | SCL | 18 |
| OLED | RST | 21 |
| OLED | PWR | 36 |
| LoRa | CS | 8 |
| LoRa | DIO1 | 14 |
| LoRa | RST | 12 |
| LoRa | BUSY | 13 |
| LoRa | SCK | 9 |
| LoRa | MISO | 11 |
| LoRa | MOSI | 10 |
| Pulsante | BUTTON | 0 |

> **Nota:** `RADIO_DIO1_PIN` è definito come `GPIO_NUM_14` per il wake-up da deep sleep.

---

## 4. Dipendenze software

| Libreria | Scopo |
|---|---|
| [RadioLib](https://github.com/jgromes/RadioLib) | Driver SX1262 |
| [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) | Web UI |
| [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library) | Grafica OLED |
| [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306) | Display OLED |
| mbedtls | AES-256-GCM |
| [PQClean](https://github.com/PQClean/PQClean) | Implementazione Kyber-512 |
| FreeRTOS | Task, code, semafori |

### Integrazione PQClean

PQClean **non è una libreria** ma una collezione di implementazioni C. Per l'ESP32 sono stati creati/adattati i seguenti file bridge:

- `pqc_kyber.h` — dichiarazioni dell'API `pqc_kyber512_keypair`, `pqc_kyber512_encapsulate`, `pqc_kyber512_decapsulate`.
- `randombytes.h` — adattato per usare il generatore hardware `esp_random()`.
- Altri file sorgente Kyber-512 leggermente modificati per compilare su ESP32.

L'API originale di PQClean per Kyber-512 è disponibile nel repository ufficiale.

---

## 5. Architettura firmware

Il firmware è organizzato in due contesti principali:

### Task radio (`radioTask`)
- **Priorità:** 2
- **Core:** 1 (pinned)
- **Stack:** 4096 byte
- **Funzioni:**
  - Svuota la coda `txQueue` e trasmette i pacchetti.
  - Riceve pacchetti con `radio.receive()`.
  - Valida STX/ETX/versione.
  - Passa i pacchetti a `handleReceivedPacket()`.

### Loop principale (`loop()`)
- Gestisce `outgoingTick()`, `rxSlotsSweep()`, `peersSweep()`, `heartbeatTick()`.
- Legge i comandi seriali.
- Aggiorna l'OLED con il contatore messaggi.
- Controlla il timeout di inattività per il deep sleep.

### Sincronizzazione

| Primitive | Uso |
|---|---|
| `stateMutex` | Mutex ricorsivo per stato condiviso (peer, outgoing, rxSlots, dedup) |
| `oledMutex` | Mutex per accesso I2C OLED |
| `txQueue` | Coda FreeRTOS di 16 elementi `TxRequest` |

---

## 6. Protocollo di comunicazione

### 6.1 Formato pacchetto (`GPacket`)

```c
struct __attribute__((packed)) GPacket {
    uint8_t  stx;           // 0x02
    uint8_t  version;       // 0x02
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
    uint8_t  payload[128];
    uint8_t  etx;           // 0x03
};
```

### 6.2 Tipi di messaggio

| Categoria | Tipi |
|---|---|
| Rete | `MSG_NET_TEST` (0x00), `MSG_HEARTBEAT` (0x01), `MSG_SAT_QUERY` (0x09), `MSG_NODE_QUERY` (0x0A), `MSG_BATT_LOW` (0x0B) |
| Emergenza | `MSG_SOS_GENERIC` (0x02), `MSG_SOS_MEDIC` (0x03), `MSG_CRIT_INJURY` (0x04), `MSG_LOST` (0x15), `MSG_GENERIC_PROBLEM` (0x12), `MSG_PATH_BLOCKED` (0x13) |
| Stato | `MSG_SUPPLY_LOW` (0x05), `MSG_STATUS_OK` (0x06), `MSG_FOUND` (0x07), `MSG_WEATHER_BAD` (0x0C), `MSG_RETURNING` (0x0D), `MSG_STATUS_FINE` (0x18) |
| Interazione | `MSG_HOW_ARE_YOU` (0x0E), `MSG_YES` (0x0F), `MSG_NO` (0x10), `MSG_DONT_KNOW` (0x11), `MSG_RECEIVED` (0x14), `MSG_OK` (0x1D) |
| Chat | `MSG_CHAT` (0x1C) |
| Controllo | `MSG_ACK` (0x08), `MSG_NACK` (0x1B) |
| PQ | `MSG_KYBER_PUBKEY` (0x20), `MSG_KYBER_CIPHERTEXT` (0x21) |

### 6.3 Flag

| Flag | Valore | Significato |
|---|---|---|
| `FLAG_ENCRYPTED` | 0x01 | Payload cifrato con AES-256-GCM |
| `FLAG_ACK_REQ` | 0x02 | Richiede ACK (solo unicast) |
| `FLAG_RELAY_OK` | 0x04 | Il pacchetto può essere relayato |
| `FLAG_BROADCAST` | 0x08 | Destinazione broadcast |

### 6.4 Trasporto affidabile

- **Frammentazione:** un messaggio è diviso in frame da 128 B, fino a 8 frame (1024 B).
- **Riassemblaggio:** ogni `IncomingMessage` usa una bitmask per tracciare i frame ricevuti.
- **ACK bitmap:** payload di 4 byte: `[sessionID_hi, sessionID_lo, mask, 0]`.
- **Retry:** timeout 3000 ms, massimo 3 tentativi, poi la sessione viene scartata.
- **Dedup:** cache circolare di 32 voci basata su hash di `sender`, `session`, `frameIdx`.

### 6.5 Mesh relay

Un pacchetto viene relayato se:

- `FLAG_RELAY_OK` è impostato.
- `ttl > 0`.
- Il pacchetto **non** è destinato a `myID` (o è broadcast).
- Il sender non è `myID`.

Il TTL viene decrementato prima della ritrasmissione.

---

## 7. Sicurezza

### 7.1 Scambio chiavi ML-KEM (Kyber-512)

Il protocollo prevede due messaggi:

1. **Initiator → Responder:** `MSG_KYBER_PUBKEY` con la chiave pubblica Kyber (800 B).
2. **Responder → Initiator:** `MSG_KYBER_CIPHERTEXT` con il ciphertext Kyber (768 B).

Entrambe le parti derivano un **shared secret di 32 byte**.

- La chiave privata Kyber (1632 B) è generata al boot e salvata in `RTC_DATA_ATTR` per sopravvivere al deep sleep.
- Lo scambio è **solo unicast** (no broadcast).

### 7.2 Cifratura AES-256-GCM con AAD

- **Chiave:** shared secret Kyber (32 B).
- **IV:** 12 byte casuali (`esp_random()`).
- **Tag:** 16 byte.
- **Overhead totale:** 28 byte.

**AAD (Additional Authenticated Data):**

L'AAD è costruito **solo** con campi che rimangono identici tra la fase di cifratura e il frame effettivamente trasmesso:

```
version || senderID || targetID || sessionID || msgType
```

> **Nota tecnica:** includere `flags`, `seq`, `timestamp` o `payloadLen` nell'AAD causava il fallimento sistematico del tag GCM in ricezione, perché questi campi cambiano tra il pacchetto "logico" e il frame ricostruito in `sendFrameOfOutgoing()`.

### 7.3 Anti-replay

- Ogni peer mantiene `last_rx_seq`.
- Un pacchetto cifrato con `seq <= last_rx_seq` viene scartato.
- Il contatore `tx_seq` viene incrementato dopo ogni cifratura riuscita.

---

## 8. Web UI

- **SSID AP:** `G-TALK-XXXXXXXX` (XXXXXXXX = ID dispositivo in hex)
- **Password AP:** `qorlyn2026`
- **HTTP Basic Auth:** `admin` / `qorlyn2026`
- **Indirizzo:** `http://192.168.4.1`

### Endpoint

| Metodo | Path | Parametri | Descrizione |
|---|---|---|---|
| GET | `/` | — | Pagina principale con form |
| POST | `/send` | `type`, `target`, `msg` | Invia messaggio |

**Tipi supportati:**

- `CHAT` → `MSG_CHAT`
- `SOS` → `MSG_SOS_MEDIC`
- `KEY` → handshake Kyber con target unicast

---

## 9. Comandi seriali

| Comando | Azione |
|---|---|
| `h` | Invia "Hello world" broadcast come `MSG_CHAT` |
| `k` | Genera keypair Kyber (se non già presente) |
| `s` | Invia "Secure hi" broadcast come `MSG_CHAT` |
| `i` | Stampa statistiche: FEC, interferenza, foreign packets, CRC error |
| `p` | Stampa lista peer con stato e ultimo seq ricevuto |

---

## 10. Gestione energia

- **Deep sleep** dopo **10 minuti** di inattività (`lastActivity`).
- **Wake-up sources:**
  - `EXT0` → pulsante su GPIO0 (livello basso).
  - `EXT1` → `RADIO_DIO1_PIN` (GPIO14) su qualsiasi livello alto.
  - Timer → 60 secondi.
- **Persistenza RTC:**
  - `g_sessionCounter`
  - `g_myID`
  - `g_kyber_pk`
  - `g_kyber_sk`
  - `g_kyber_ready`

---

## 11. Build e integrazione PQClean

### Prerequisiti

- Arduino IDE o PlatformIO con supporto ESP32.
- Board: ESP32 (es. ESP32 Dev Module).
- Partizioni: default con SPIFFS (non usato) o minimal.

### File bridge PQClean

Per compilare su ESP32 è necessario includere i file adattati:

```
pqc_kyber.h
randombytes.h
kyber512/
  ├── clean.c
  ├── ...
  └── ...
```

Il file `randombytes.h` deve implementare `randombytes(uint8_t *output, size_t n)` usando `esp_random()` o un'altra sorgente hardware affidabile.

### Note di compilazione

- Assicurarsi che `mbedtls/gcm.h` sia disponibile (incluso nell'ESP32 Arduino core).
- Verificare che `FSPI` sia disponibile (ESP32 classico).
- La dimensione di `GPacket` è allineata a 1 byte (`__attribute__((packed))`).

---

## 12. Struttura file

```
Qorlyn_v2.ino
pqc_kyber.h
randombytes.h
kyber512/            # File PQClean adattati
  ├── clean.c
  ├── clean.h
  ├── ...
  └── ...
```

---

## 13. Limitazioni note

1. **Cifratura solo su singolo frame:** se `len + 28 > 128`, il messaggio viene inviato in chiaro.
2. **Handshake Kyber solo unicast:** il broadcast non è supportato per lo scambio chiavi.
3. **Nessuna forward secrecy:** il shared secret rimane statico finché il peer è in memoria.
4. **Cache peer limitata:** massimo 8 peer, 3 outgoing, 6 rx slot.
5. **Dedup cache volatile:** non persiste al deep sleep.

---

## 14. Issues
See [Issues Page](https://github.com/qorlyn/firmware/issues).

---

## 15. Licenza

- **Qorlyn Project:** GPL V3. See [LICENSE](./LICENSE) File.
- **PQClean:** vedere il repository ufficiale per i termini di licenza.

---

## 15. Riferimenti

- [PQClean — Clean, portable, tested implementations of post-quantum cryptography](https://github.com/PQClean/PQClean)
- [RadioLib — Universal wireless communication library](https://github.com/jgromes/RadioLib)
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
- [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306)

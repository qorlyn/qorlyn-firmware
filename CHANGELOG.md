# Changelog

All notable changes to the Qorlyn firmware.
The format follows [Keep a Changelog](https://keepachangelog.com/it/1.1.0/).
Versioning follows [Semantic Versioning](https://semver.org/lang/it/). ## [Unreleased]

### Added
- Peer state machine with 4 states (`PEER_NONE`, `PEER_PENDING_IN`, `PEER_PENDING_OUT`, `PEER_ESTABLISHED`)
- Peer acceptance policies: `manual`, `auto`, `whitelist`, `hybrid`
- Blocklist and whitelist persistent in RTC memory
- Multi-layer onion routing with per-hop ephemeral X25519 (up to 6 hops)
- End-to-end ACK for onion routing using an encrypted opaque token
- Multi-protocol transport: LoRa + WiFi STA/AP + BLE peripheral/central
- Automatic transport selection per peer (`lastSeenProto`)
- Plaintext fallback notification on OLED and Web UI
- Full Web UI with management for peers, policies, whitelists, blocklists, and onion routing
- Extended serial commands (`H`, `C`, `S`, `B`, `O`, `M`, `p`, `P`, `W`, `i`, `F`, `X`, `Z`, `Y`, `R`, `k`, `x`)
- Periodic heartbeat with X25519 public key broadcast (every 30s)
- AAD on encrypted packets to bind ciphertext to the header
- Multi-frame handshake broadcasting across all active protocols
- RTC magic value to distinguish cold boot from warm reset
- Dedicated `mainTask` with 16 KB stack
- `bleRxQueue` for deferred BLE packet processing

### Changed
- `OLED_PWR` moved from GPIO 32 to GPIO 36 (avoids PSRAM conflict)
- WiFi initialized in pure `WIFI_AP` mode, not `AP_STA`, at boot
- `transmitFrame` recognizes handshakes and broadcasts across all protocols
- `sendAckBitmap` uses `g_lastRxProto` instead of `selectProtoFor`
- Onion routing: - Skip peer check for `path[i] == myID`
- Plaintext warning moved to serial output only (no longer displayed on OLED)

### Fixed
- Boot loop caused by `OLED_PWR=32` conflicting with octal PSRAM
- `assert failed: tcp_alloc` crash during AsyncWebServer initialization
- `mbedtls_md5_starts_ret` undeclared (renamed in mbedtls 3.x)
- `Stack canary watchpoint triggered (loopTask)` crash
- `Stack canary watchpoint triggered (nimble_host)` crash during Kyber decapsulation
- Unreliable multi-frame handshake over LoRa due to packet loss
- ACK sent over the wrong transport during BLE handshake
- SOS sent in plaintext due to `nullptr` rejected by mbedtls
- Onion loopback failed with `no x25519 pk` when target == self
- SOS not displayed on OLED due to `updateUI` throttling

### Removed
- Redundant old `PacketFlags` enum
- Unused buffers in the handshake path

---

## Previous version history ("G-Mesh v2" prototype)

### Unversioned — initial prototype
- LoRa transport only
- Kyber-512 handshake without user consent
- AES-256-GCM encryption without AAD
- No onion routing
- Minimal Web UI
- Peers represented as boolean flags (`inUse`, `established`)

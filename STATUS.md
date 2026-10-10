# Status

Current status of the Qorlyn firmware, verified on actual hardware.
Last update: 10/10/2026.

## Test hardware

- 2× Heltec LoRa 32 V3.2 (ESP32-S3 + SX1262 + SSD1306)
- Power supply: USB (external charger recommended to avoid brownouts)
- Toolchain: Arduino IDE 2.x, ESP32 core 3.x

## Summary

| Area | Status |
|---|---|
| Compilation | ✅ Working |
| Clean boot (no WDT, no crash) | ✅ Working |
| SX1262 LoRa radio | ✅ Working |
| SSD1306 OLED | ✅ Working |
| WiFi AP + Web UI | ✅ Working |
| WiFi STA discovery | ❌ Disabled |
| BLE peripheral | ✅ Working |
| BLE central | ✅ Working |
| Kyber-512 handshake | ✅ Working |
| AES-256-GCM encrypted chat | ✅ Working |
| X25519 pk exchange | ✅ Working |
| Encrypted SOS | ⚠️ Fixed, needs re-testing |
| Onion loopback | ⚠️ Fixed, needs re-testing |
| Onion multi-hop (3+) | ❌ Not tested |
| Deep sleep | ❌ Not tested |
| 24h stability | ❌ Not tested | ## What works (verified on hardware)

### Initialization
- Clean boot on both boards
- LoRa radio initialized and receiving
- Kyber-512 keypair generated
- X25519 keypair generated
- WiFi AP active with SSID `QORLYN-XXXXXXXX`
- Web UI accessible at `http://192.168.4.1`
- BLE peripheral advertising
- BLE central connected to peer

### Kyber-512 handshake via BLE
- 7 public key frames transmitted and received correctly
- 6 ciphertext frames transmitted and received correctly
- Peer state machine transitions correctly
- `manual` policy requires explicit approval
- X25519 public key exchanged in both directions

### Encrypted chat
- Message encrypted with AES-256-GCM
- AAD binds ciphertext to the header
- Successful decryption on the receiving end
- No plaintext warning when session is active

### Peer management
- Peer list showing status, protocol, and X25519 status
- Persistent blocklist and whitelist
- Policy toggle (`M`)
- Forget peer (`F`)
- Block peer (`X`)
- Full reset (`Z`)

## What does not work or is disabled

### WiFi STA discovery
`wifiStaTry()` has a `return;` statement at the beginning.
Reason: `WiFi.scanNetworks()` caused a stack overflow in `loopTask`.
The 16 KB `mainTask` fix should allow re-enabling it, but this has not been tested.

### BLE + WiFi coexistence
Known ESP32 hardware issue: BLE central and WiFi STA share the 2.4 GHz radio.
When BLE is continuously active, the WiFi STA connection drops.
Unresolved. Possible approaches: time-slicing, dynamic priority, or mutual disabling.

### Multi-hop onion routing
Never tested. Requires 3+ boards with pre-established Kyber sessions between all pairs along the path.

### Deep sleep
The `enterSleep()` code is present but untested following recent fixes.

### Long-term stability
All tests were short (minutes). No continuous 24-hour test has been performed.

## Tests performed

| # | Test | Outcome |
|---|---|---|
| 1 | Clean boot on 2 boards | ✅ |
| 2 | LoRa + Kyber + X25519 initialization | ✅ |
| 3 | WiFi AP + Web UI | ✅ |
| 4 | BLE peripheral + central | ✅ |
| 5 | Mutual detection via BLE | ✅ |
| 6 | Kyber-512 handshake (7 PK frames + 6 CT frames) | ✅ |
| 7 | X25519 PK exchange | ✅ |
| 8 | End-to-end encrypted chat | ✅ |
| 9 | Peer list (`p`) | ✅ |
| 10 | Encrypted SOS | ⚠️ Fixed, needs re-testing |
| 11 | Onion loopback | ⚠️ Fixed, needs re-testing |
| 12 | WiFi STA discovery | ❌ Disabled |
| 13 | Multi-hop onion | ❌ Untested |
| 14 | Deep sleep | ❌ Untested |

## Known issues (open)

| # | Issue | Severity | Proposed fix |
|---|---|---|---|
| 1 | BLE central kills WiFi STA | High | Time-slicing or mutual disabling |
| 2 | `WiFi.scanNetworks()` blocks the stack | Medium | Asynchronous scan |
| 3 | `lastSeenProto` can be stale | Low | Periodic reset (already implemented, untested) |
| 4 | Fragile BLE election | Low | More robust logic with tie-breaker |
| 5 | Brownout with PC USB | Medium | External charger |

## Resolved issues

| # | Issue | Fix |
|---|---|---|
| 1 | WDT reset loop | OLED_PWR from GPIO 32 to 36 |
| 2 | `assert tcp_alloc` | WiFi in pure AP mode |
| 3 | Missing `mbedtls_md5_starts_ret` | Replaced with `mbedtls_md5_starts` |
| 4 | AsyncTCP crash | ESP32Async fork ≥ 3.3.3 |
| 5 | loopTask stack overflow | `mainTask` 16 KB |
| 6 | Unreliable multi-frame handshake | Multi-proto broadcast |
| 7 | ACK on wrong proto | `sendAckBitmap` via `g_lastRxProto` |
| 8 | nimble_host stack overflow | `bleRxQueue` + mainTask queue |
| 9 | Plaintext SOS | Pass `&empty` instead of `nullptr` |
| 10 | Onion self-loop | Skip check for `path[i]==myID` |
| 11 | SOS not on OLED | Warning on serial only |

## Serial commands

Connection: 115200 baud, line ending `NL and CR`.

| Command | Effect |
|---|---|
| `H <id>` | Kyber handshake to id |
| `a` / `r` | Accept / reject first pending |
| `A` | List pending-in |
| `C <id> <msg>` | Encrypted chat |
| `S <id>` | Medical SOS |
| `B <msg>` | Broadcast | | `O <id> <hops> <msg>` | Onion routing |
| `M` | Toggle policy |
| `p` / `P` | Peer List / Policy |
| `W` | State of transport |
| `i` | Interference info |
| `F <id>` | Forget peer |
| `X <id>` | Block peers |
| `Z` | Reset peers and pending |
| `Y` | Toggle BLE central scan |
| `R` | Re-init LoRa radio |
| `h` | Hello broadcast |
| `k` / `x` | Kyber / X25519 keypair |
| `?` | Help |

# Roadmap

Planned next steps for Qorlyn firmware.
Items are listed by priority. ## Short term (immediate)

### Testing and verification
- [ ] Retest **encrypted SOS** after `&empty` fix
- [ ] Retest **onion loopback** after `path[i]==myID` fix
- [ ] Regression test on **encrypted chat**
- [ ] Verify that peer state remains `ESTABLISHED` during operations

### Re-enabling features
- [ ] Re-enable `wifiStaTry()` and verify that the 16 KB `mainTask` supports it
- [ ] Test end-to-end WiFi STA discovery
- [ ] Verify no crashes occur when both BLE and WiFi are active

## Medium term

### Radio coexistence
- [ ] Resolve BLE + WiFi coexistence:
- time-slicing: alternate WiFi and BLE every N ms
- dynamic priority based on target peer
- automatic fallback to other protocol
- [ ] Replace blocking `WiFi.scanNetworks()` with asynchronous scan

### Peer robustness
- [ ] Improve `lastSeenProto` logic with hysteresis
- [ ] Add explicit timeout for each protocol
- [ ] Improve BLE election with RSSI-based tie-breaker

### State management
- [ ] Serial command for full state dump (peers, rxSlots, outgoing, onion tracker)
- [ ] Circular event log accessible via Web UI
- [ ] Command for emulated cold boot (full RTC reset without disconnecting power)

## Long term

### Onion routing
- [ ] Test with 3+ boards for true multi-hop onion routing
- [ ] Test with 6 hops (maximum path)
- [ ] Verify end-to-end ACK on multi-hop paths

### Stability
- [ ] 24-hour stability test
- [ ] Deep sleep test with wake-up from... radio/button/timer
- [ ] SOS test without session (plaintext fallback)
- [ ] End-to-end blocklist/whitelist test

### Documentation
- [ ] Documented threat model
- [ ] Onion routing flow diagram
- [ ] Peer state machine diagram
- [ ] Multi-protocol architecture diagram

### Web UI
- [ ] Real-time event log display
- [ ] Peer ping/status
- [ ] Historical RSSI/SNR graph
- [ ] Radio parameter configuration (frequency, SF, power)

## Future features (out of current scope)

These features require additional hardware or major architectural decisions. ### Additional hardware required
- [ ] **GPS** (UART module, e.g., NEO-6M) — SOS location
- [ ] **BME280** (I2C sensors) — temperature, humidity, pressure
- [ ] **PWM fans** — temperature control
- [ ] **Physical privacy switch** — GPIO input
- [ ] **20+ W radio module** — extended range
- [ ] **PUF** (dedicated chip like ATECC608A) — silicon-derived keys

### Architectural features
- [ ] **Adaptive frequency hopping** — true anti-jamming
- [ ] **Dynamic power adjustment** — max power for SOS, reduced for chat
- [ ] **OTA** — firmware updates via WiFi
- [ ] **Codec2** — short voice messages
- [ ] **Bluetooth Mesh** (non-GATT) — if applicable to the use case

### Optional features
- [ ] **Multi-hop ACK** without path disclosure
- [ ] **Forward secrecy sessions** (periodic re-keying)
- [ ] **Handshake authentication** (PSK or signature) against MITM
- [ ] **Topological discovery** — optimized routing

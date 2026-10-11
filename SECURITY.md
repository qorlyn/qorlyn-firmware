# Security Policy

## Project Status

Qorlyn is experimental firmware currently under active development. **It has not undergone an independent security audit.** The code is publicly available, but no professional cryptographer has reviewed it to date.

Anyone requiring guaranteed security for critical communications **should not use Qorlyn in production** without first:
- reading the source code
- understanding the threat model
- evaluating the limitations stated below
- potentially conducting an independent audit

## Supported Versions

| Version | Supported | Notes |
|---|---|---|
| v0.1.x | ✅ Yes | Initial public release |
| Previous (prototype) | ❌ No | No longer maintained |

Only the latest release receives security updates.

## How to Report a Vulnerability

**Do not open a public issue for security problems.**

Contact directly:

- 📧 Email: `security@qorlyn.org`
- 🔐 PGP: public key available on [keys.openpgp.org](https://keys.openpgp.org) (fingerprint: to be published)

In your report, please include:
- description of the issue
- steps to reproduce it
- estimated impact
- any PoC (handle with care)
- whether you wish to be mentioned in the credits

**Expected response times:**
- Acknowledgement of receipt: within 72 hours
- Initial assessment: within 7 days
- Fix or mitigation: depends on severity; typically within 30 days for critical issues

We do not currently have a bug bounty program. Contributors are mentioned in the release credits if they wish. ## Scope

### In scope

- Qorlyn firmware for Heltec LoRa 32 V3.2
- Network protocol (handshake, encryption, routing)
- Web UI
- Serial interface
- Transports: LoRa, WiFi, BLE
- RTC persistence
- PQClean

### Out of scope

- Hardware (Heltec, Espressif, Semtech): report to manufacturers
- Third-party libraries (mbedTLS, RadioLib, ESPAsyncWebServer): report to their maintainers
- Physical attacks (side-channel, fault injection, direct flash reading)
- Attacks requiring unmitigated physical access
- Generic DoS via radio jamming (inherent to any radio)
- Security issues already known and documented in `STATUS.md`
- Theoretical weaknesses without a demonstrable practical exploit

## Cryptographic model

### Algorithms used

| Role | Algorithm | Quantum-resistant |
|---|---|---|
| Key encapsulation | ML-KEM Kyber-512 | ✅ Yes (NIST cat. 1) |
| Symmetric encryption | AES-256-GCM | ✅ Yes (128-bit post-quantum via Grover) |
| Authentication | AES-GCM tag | ✅ Yes |
| Onion layer keys | Ephemeral per-hop X25519 | ❌ No (vulnerable to Shor) |
| RNG | `esp_fill_random` (hardware) | — |

### Rationale for this combination

Kyber-512 is a post-quantum KEM. It is used **only once per session** to establish a 32-byte secret. That secret becomes the AES-256-GCM key for all subsequent messages. This hybrid scheme is the de facto standard (TLS 1.3, Signal, WireGuard) because:

- PQC is used where needed (key exchange)
- Symmetric encryption is used where speed is required (data)
- AES-256 with hardware acceleration comes at virtually no cost

### What is NOT quantum-resistant

**Onion routing uses X25519**, which is based on elliptic curves and is therefore vulnerable to Shor's algorithm. A future fault-tolerant quantum computer could decrypt onion layers of previously recorded traffic (a "harvest now, decrypt later" attack).

This is a **conscious trade-off**: using Kyber for every onion layer would increase packet size by 10–20× and latency beyond what LoRa can support. The alternative is to document the limitation and accept it.

If you require long-term anonymity against quantum adversaries, **do not use Qorlyn's onion routing**.

## Known security limitations

These limitations are **consciously accepted** in this version. They are listed here so that anyone evaluating Qorlyn is aware of them before use.

### 1. No forward secrecy

Session keys (Kyber shared secret, X25519 ephemeral) **persist in RTC memory** across reboots. If an attacker compromises the device and reads `g_kyber_sk` or `g_x25519_sk`, they can decrypt all past sessions for which they have recorded traffic.

**Planned mitigation (roadmap):** periodic key rotation (every 24 hours or every N sessions).

### 2. Unauthenticated handshake

The Kyber exchange is not signed. An attacker with access to the channel (man-in-the-middle) can impersonate a peer and establish two separate sessions, one with each side.

**Planned mitigation (roadmap):** Pre-shared key (PSK) or long-term key signature.

### 3. Onion routing is not PQC

As explained above: X25519 is classical.

### 4. `lastSeenProto` can be manipulated

A nearby attacker can send packets on a specific proto to make the device believe a peer is reachable via that proto, causing a DoS (waiting on the wrong proto).

**Severity:** Low. The system has timeouts and fallbacks.

### 5. The message in plaintext transmission is silent
Plaintext transmission is silent

If a session is not established, or if the payload exceeds 100 bytes, the message is sent in plaintext. The user is notified (via OLED and Web UI), but the message is **sent regardless**. This is by design (broadcasts and emergency messages must go through even without a session), but it is something to keep in mind.

### 6. Basic rate limiting

Handshake rate limiting is set to 2 seconds per peer, with a global limit of 4 pending requests. There is no protection against an attacker sending public keys from multiple fake IDs.

**Severity:** Low. The system operates on LoRa, which is inherently bandwidth-constrained.

### 7. Web UI uses HTTP Basic Auth

Default username: `admin`; default password: `gmesh2026`. Anyone connected to the device's AP network can attempt to log in.

**Planned mitigation:** Configurable password on first boot; enforcing a change from the default.

### 8. No secure boot / flash encryption

The firmware is unsigned. An attacker with physical access could load modified firmware without the bootloader detecting it.

**Out of scope** for this version. This requires ESP32 eFuse configuration, which is a one-time, irreversible operation. ## User Best Practices

If you use Qorlyn for actual communication:

1. **Change the Web UI password** from the default (`admin`/`gmesh2026`)
2. **Update frequently** when new releases are available
3. **Do not use broadcast for sensitive messages** (they are sent in the clear)
4. **Verify the peer's fingerprint** out-of-band (verbally or in person) if possible
5. **Do not trust a peer simply because the handshake completed**: the handshake does not provide authentication
6. **Assume the device is compromised** if you lose it or if someone has had physical access to it for more than a few minutes
7. **Do not use onion routing for content that must remain sensitive for decades** against a long-term quantum adversary

## Critical Dependencies

The firmware relies on:

- **mbedTLS** (part of the ESP32 core) — AES-GCM and ECP X25519 implementation
- **pqc_kyber** — PQClean reference implementation of Kyber-512
- **RadioLib** — SX1262 driver
- **ESPAsyncWebServer** — HTTP server
- **Adafruit SSD1306 / GFX** — display driver

Vulnerabilities in these dependencies affect Qorlyn. Update regularly.

## Related Documents

- `STATUS.md` — current status and open issues
- `ROADMAP.md` — planned security fixes
- `CHANGELOG.md` — change history

## Acknowledgments

Individuals who responsibly report security issues will be publicly acknowledged in the release containing the fix, if they so desire. Anonymous reports are accepted.

The current version of Qorlyn is in Alpha (0.1.0). The firmware is subject to change.

Last update: 11/10/2026 12:12 PM - Italian time
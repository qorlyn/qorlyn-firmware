# <img src="assets/qorlyn-symbol.svg" width="32" alt="Qorlyn"> QORLYN

### The network that remains when the rest stops.

**Quantum Onion Routing Lattice lYnx Network**

Decentralized, post-quantum, censorship-resistant mesh communication.
Born on the field. For those who have no alternatives.

<br>

[![Website](https://img.shields.io/badge/WEBSITE-qorlyn.org-00B8D4?style=for-the-badge&logo=firefox&logoColor=white)](https://qorlyn.org)
[![License](https://img.shields.io/badge/LICENSE-AGPL--3.0-5C8B2F?style=for-the-badge&logo=gnu&logoColor=white)](LICENSE)
[![Status](https://img.shields.io/badge/STATUS-ALPHA-E8A800?style=for-the-badge)](STATUS.md)
[![Version](https://img.shields.io/badge/VERSION-0.1.0--alpha-0A0A0A?style=for-the-badge)](CHANGELOG.md)

[![Platform](https://img.shields.io/badge/PLATFORM-ESP32--S3-2A7FA8?style=for-the-badge&logo =espressif&logoColor=white)](https://www.espressif.com/en/products/socs/esp32-s3)
[![Radio](https://img.shields.io/badge/RADIO-LoRa%20868%20MHz-5C8B2F?style=for-the-badge)](https://qorlyn.org/docs/radio)
[![PQC](https://img.shields.io/badge/PQC-KYBER--512-00B8D4?style=for-the-badge)](https://qorlyn.org/docs/crypto)
[![Onion](https://img.shields.io/badge/ONION-X25519%20PER--HOP-0F3A52?style=for-the-badge)](https://qorlyn.org/docs/onion)

[![Server](https://img.shields.io/badge/SERVER-ZERO-0A0A0A?style=for-the-badge)](https://qorlyn.org/manifesto)
[![Not [![Non-Profit](https://img.shields.io/badge/NON--PROFIT-100%25-5C8B2F?style=for-the-badge)](https://qorlyn.org/manifesto)
[![Open Source](https://img.shields.io/badge/OPEN-SOURCE-00B8D4?style=for-the-badge)](https://github.com/qorlyn)
[![PRs](https://img.shields.io/badge/PRs-WELCOME-00B8D4?style=for-the-badge)](CONTRIBUTING.md)
[![Made in Italy](https://img.shields.io/badge/MADE%20IN-ITALY-0A0A0A?style=for-the-badge)](https://qorlyn.org)

<br>

[**Manifesto**](https://qorlyn.org/manifesto) · [**What is it?**](#what-is-qorlyn) · [**How ​​it works**](#how-it-works) · [**Devices**](#devices) · [**Download**](#download) · [**Contribute**](CONTRIBUTING.md) · [**Contact**](#contact)

---

<img src="assets/banner.png" alt="Qorlyn — The network that remains when the rest stops" width="100%">

---

## What is Qorlyn?

There is a precise moment when you realize that the world you were promised doesn't exist.

You are in a war zone, or under a regime that has just shut down the internet, or in a refugee camp after an earthquake. Cell towers have fallen. Your SIM has no signal. Satellite connections cost three thousand euros and track your location.

You need to send a message. Just a few words. A snippet of info, a coordinate, an "I'm alive."

Your phone is useless. Standard radio broadcasts are unencrypted; anyone can listen. Messaging apps require the internet, and the internet isn't available.

**Qorlyn exists for that moment.**

Qorlyn is a **decentralized, anonymous, post-quantum** communication protocol. It operates without the internet, SIM cards, servers, or permissions. Every device is a node. Every node is an equal peer. The network forms where it is needed and dissolves when no longer required.

It is not a commercial product. It is an **open standard** licensed under [AGPL-3.0](LICENSE). The code is public. Anyone can verify, modify, or distribute it. No one can shut it down, sell it as proprietary software, or insert a backdoor.

> **Qorlyn is the network. Lynx is the device. The protocol belongs to everyone.**

---

## The Problem

In 2026, we are promised freedom, safety, and a future-ready existence.
Yet censorship remains archaic; we are responding from the future—the very future we were promised.

**43 journalists** killed in 2024, many while attempting to communicate.
**12 countries** with active information blackouts.
**1.2 billion people** under mass surveillance.

**Qorlyn is the technical answer to a human problem.**

---

## How It Works

### Post-quantum cryptography

Kyber-512, the NIST standard designed to withstand future quantum computers. Each session uses ephemeral keys that are destroyed after use. Even if someone records the traffic today, they will be unable to decrypt it tomorrow.

### Onion routing

Each packet passes through three random nodes before reaching its destination. Each hop knows only the preceding and succeeding nodes. No single node sees both the sender and the recipient. Tor model, adapted for radio.

### LoRa Transport

Spread-spectrum modulation, license-free ISM band (868 MHz in Europe). Up to 12 km range in open terrain with 1.5W. Operates below the noise floor. Jamming-resistant due to the nature of the modulation.

[**Learn more about the architecture →**](https://qorlyn.org/docs/architecture)

---

## Physical wipe

<img src="assets/wipe-diagram.svg" alt="Physical wipe: chip destruction in the event of seizure" width="100%">

If you get caught, if your device is seized, if they try to forcibly extract the keys from the silicon — **you pull a cord**. The chip is physically destroyed. Not just the keys. The processor, the memory, everything. In less than two seconds.

There is nothing left to recover, because there is nothing left.

HSMs do this on servers. Phones have a "wipe data" function. No commercial portable device has a cord that physically destroys the silicon.

---

## Devices

Four versions, same protocol. A Vanilla and a BlackSil speak the same language. Same encryption. Same network.

<table>
<tr>
<td align="center" width="25%">
<img src="assets/device-vanilla.png" alt="Qorlyn Vanilla"><br>
<b>Qorlyn Vanilla</b><br>
<sub>The entry-level model</sub>
</td>
<td align="center" width="25%">
<img src="assets/device-lynx.png" alt="Qorlyn Lynx"><br>
<b>Qorlyn Lynx</b><br>
<sub>Tested, more robust</sub>
</td>
<td align="center" width="25%">
<img src="assets/device-blackout.png" alt="Qorlyn Lynx Blackout"><br>
<b>Qorlyn Lynx Blackout</b><br>
<sub>For serious users</sub>
</td>
<td align="center" width="25%">
<img src="assets/device-blacksil.png" alt="Qorlyn BlackSil"><br>
<b>Qorlyn BlackSil</b><br>
<sub>For those requiring absolute privacy</sub>
</td>
</tr>
</table>

**Qorlyn Vanilla** — Heltec V3.2 board, membrane keypad, 3D-printed case. Does everything needed for communication. For civilians, students, and protesters.

**Qorlyn Lynx** — Custom PCB, larger battery, IP54-rated case. For activists, freelance journalists, and hikers.

**Qorlyn Lynx Blackout** — Physically removable GPS and microphone, dual battery, IP67-rated case, emergency wipe function. For journalists at risk and personnel in critical areas.

**Qorlyn BlackSil** — PUF, secure boot, comprehensive sensors, integrated solar panel, physical wipe. For war reporters, NGOs, and private security.

[**Full specifications →**](https://qorlyn.org/devices)

---

## What Qorlyn does NOT promise

We would rather lose a reader than mislead those who trust us.

- **It is not a satellite phone.** It does not transmit 4K video. It does not make calls to the other side of the world.
- **LoRa bandwidth is narrow.** 20 kilobits per second. Text, coordinates, SOS, low-quality compressed audio.
- **Absolute anonymity does not exist.** We make an attacker's job difficult, not impossible.
- **Onion routing hides the recipient, not the sender.** The first hop sees who sent the message but does not know if they are the original sender.
- **Frequencies depend on local regulations.** 868 MHz in Europe, 915 MHz in the USA.
- **Qorlyn is a tool, not a miracle.**

The complete [threat model](https://qorlyn.org/threat-model) is public. Read it before using Qorlyn in critical scenarios.

---

## Downloads

### Firmware

| Channel | Version | Link |
|---|---|---|
| **Stable** | v0.1.0-alpha | [Download](https://github.com/qorlyn/qorlyn-firmware/releases/latest) |
| **Development** | main branch | [Clone the repo](https://github.com/qorlyn/qorlyn-firmware) |
| **Archive** | All releases | [Releases](https://github.com/qorlyn/qorlyn-firmware/releases) |

### Getting Started

1. **Read the [installation guide](https://qorlyn.org/docs/install)**
2. **Check the [hardware requirements](https://qorlyn.org/docs/hardware)**
3. **Follow the [first boot guide](https://qorlyn.org/docs/first-boot)**
4. **Join the [Discord](https://qorlyn.org/discord)** for support

### Project Files

- [`CHANGELOG.md`](CHANGELOG.md) — Version history
- [`STATUS.md`](STATUS.md) — Current status and known bugs
- [`ROADMAP.md`](ROADMAP.md) — Upcoming features
- [`CONTRIBUTING.md`](CONTRIBUTING.md) — How to contribute
- [`CODE_OF_CONDUCT.md`](CODE_OF_CONDUCT.md) — Community guidelines
- [`LICENSE`](LICENSE) — AGPL-3.0

---

## Current Status

**Version:** v0.1.0-alpha

**Working / Verified on hardware:**
- Kyber-512 handshake via BLE
- X25519 key exchange
- End-to-end encrypted chat
- Single-hop onion routing
- Full Web UI
- Peer management with policies, whitelists, and blocklists
- RTC persistence

**Not verified / Not yet working:**
- Multi-hop onion routing
- WiFi STA discovery (stack issue)
- Deep sleep
- Voice (Codec2, in development)
- GPS, sensors (in development)

**Coming soon:**
- OTA firmware updates
- PUF, secure boot
- PCB production
- Hardware device certifications
Full details in [`STATUS.md`](STATUS.md) and [`ROADMAP.md`](ROADMAP.md). ---

## Contributing

Qorlyn is open source under the [AGPL-3.0](LICENSE) license. Anyone can contribute: bug fixes, hardware testing, documentation, translations, and security audits.

**What we are looking for:**
- Firmware bug fixes
- Testing on real hardware (mandatory for every PR)
- Translations (English, Spanish, Arabic, Farsi, Ukrainian, Russian)
- Cryptographic code audits

**What we are not looking for:**
- Complete rewrites without prior discussion
- Untested or misunderstood AI code (we accept AI-generated code, but not code that hasn't been reviewed, understood, and debugged)
- Backdoors, telemetry, or tracking

Full guidelines in [`CONTRIBUTING.md`](CONTRIBUTING.md).

---

## FAQ

**Is an internet connection required?** No. Never.
**Is a SIM card required?** No. Never.
**Is a server required?** No. There is no Qorlyn server.
**Is it legal?** Yes, in most countries.

Check local laws.
**Can I be tracked?** Content is encrypted. Metadata is partially hidden via onion routing.
**What is the range?** 10–15 km per hop in open terrain; 1–3 km in urban areas.
**Can I use voice?** Not yet. Voice functionality is under development.
**Who is behind it?** An independent developer. The project is designed to be forkable and community-maintained. [**More questions →**](https://qorlyn.org/faq)

---

## Contacts

[![Website](https://img.shields.io/badge/Website-qorlyn.org-00B8D4?style=flat-square)](https://qorlyn.org)
[![Email](https://img.shields.io/badge/Email-qorlyn@qorlyn.org-5C8B2F?style=flat-square)](mailto:qorlyn@qorlyn.org)
[![Discord](https://img.shields.io/badge/Discord-qorlyn.org%2Fdiscord-2A7FA8?style=flat-square)](https://qorlyn.org/discord)
[![Telegram](https://img.shields.io/badge/Telegram-qorlyn.org%2Ftelegram-4A9ECD?style=flat-square)](https://qorlyn.org/telegram)
[![GitHub](https://img.shields.io/badge/GitHub-github.com%2Fqorlyn-0A0A0A?style=flat-square)](https://github.com/qorlyn)
[![Forum](https://img.shields.io/badge/Forum-forum.qorlyn.org-5C8B2F?style=flat-square)](https://forum.qorlyn.org)

**Dedicated channels**
- **Security:** [security@qorlyn.org](mailto:security@qorlyn.org) — [full policy](https://qorlyn.org/security)
- **Conduct:** [conduct@qorlyn.org](mailto:conduct@qorlyn.org) — [code of conduct](CODE_OF_CONDUCT.md)
- **Press:** [press@qorlyn.org](mailto:press@qorlyn.org)

**Support the project**
- [Donations](https://qorlyn.org/donate)
- [Sponsors](https://github.com/sponsors/qorlyn)

---

<div align="center">

<img src="assets/qorlyn-symbol.svg" width="48" alt="Qorlyn">

**QORLYN**

*The network that remains when the rest stops.*

<sub>AGPL-3.0-or-later · Non-profit · Open source · Made in Italy</sub>

</div>

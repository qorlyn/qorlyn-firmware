# Qorlyn — Press Kit

**The network that remains when the rest stops.**

---

## Overview

**Qorlyn** is a decentralized, post-quantum, censorship-resistant mesh communication protocol. It operates without internet, SIM cards, servers, or permissions. Every device is a node. Every node is an equal peer. The network forms where it is needed and dissolves when no longer required.

Born from the need for truly secure, independent, and low-cost communication in war zones, during natural disasters, or under regimes with high levels of censorship, Qorlyn is an open standard licensed under AGPL-3.0.

> **Qorlyn is the network. Lynx is the device. The protocol belongs to everyone.**

---

## Fast Facts

| | |
|---|---|
| **Project Name** | Qorlyn (formerly G-Mesh) |
| **Founder** | Emanuele Ferraro (independent developer) |
| **Founded** | April 2026 (first domain g-mesh.org registered 25 April 2026) |
| **Headquarters** | Italy |
| **Status** | Alpha (v0.1.0-alpha) |
| **License** | AGPL-3.0 |
| **Hardware Tested** | Heltec Meshtastic LoRa 32 v3.2 |
| **Tests Passed** | 70+ |
| **Active Nodes** | 0 (pre-release) |
| **GitHub Stars** | 0 (new project) |
| **Website** | https://qorlyn.org |
| **Repository** | https://github.com/qorlyn/qorlyn-firmware |

---

## About the Founder

Qorlyn is developed by **Emanuele Ferraro**, a thirteen-year-old independent developer based in Italy. Emanuele is a firm believer in the fundamental right to freedom of expression and communication, even when local authorities do not permit it. Through projects like **anticensura.org**, he has contributed to making communication free and accessible.

The idea for Qorlyn was born from **GeoReport**, a platform Emanuele led, which analyzed global events and identified a critical gap: there was no truly secure, independent, low-cost, and low-power way to communicate when infrastructure fails.

---

## The Problem

In 2026, we are promised freedom, safety, and a future-ready existence. Yet censorship remains archaic.

- **43 journalists** killed in 2024, many while attempting to communicate.
- **12 countries** with active information blackouts.
- **1.2 billion people** under mass surveillance.

**Qorlyn is the technical answer to a human problem.**

---

## How It Works

### Post-Quantum Cryptography
Kyber-512, the NIST standard designed to withstand future quantum computers. Each session uses ephemeral keys that are destroyed after use.

### Onion Routing
Each packet passes through three random nodes before reaching its destination. No single node sees both the sender and the recipient. Tor model, adapted for radio.

### LoRa Transport
Spread-spectrum modulation, license-free ISM band (868 MHz in Europe). Up to 12 km range in open terrain with 1.5W. Operates below the noise floor. Jamming-resistant.

### Physical Wipe
If a device is seized, pulling a cord physically destroys the chip — processor, memory, everything — in less than two seconds. No commercial portable device offers this.

*Full technical details are available in the official documentation: https://qorlyn.org/docs*

---

## Devices

Four versions, same protocol. A Vanilla and a BlackSil speak the same language. Same encryption. Same network.
Devices form the Lynx family:

| Device | Target User | Key Features |
|---|---|---|
| **Qorlyn Lynx Vanilla** | Civilians, students, protesters | Heltec V3.2 board, membrane keypad, 3D-printed case |
| **Qorlyn Lynx** | Activists, freelance journalists, hikers | Custom PCB, larger battery, IP54-rated case |
| **Qorlyn Lynx Blackout** | Journalists at risk, critical personnel | Removable GPS/microphone, dual battery, IP67, emergency wipe |
| **Qorlyn Lynx BlackSil** | War reporters, NGOs, private security | PUF, secure boot, solar panel, physical wipe |

**Full specifications:** https://qorlyn.org/devices

---

## What Qorlyn Does NOT Promise

We would rather lose a reader than mislead those who trust us.

- **It is not a satellite phone.** No 4K video, no international calls.
- **LoRa bandwidth is narrow.** 20 kbps. Text, coordinates, SOS, low-quality compressed audio.
- **Absolute anonymity does not exist.** We make an attacker's job difficult, not impossible.
- **Onion routing hides the recipient, not the sender.**
- **Frequencies depend on local regulations.**
- **Qorlyn is a tool, not a miracle.** Read the threat model before using it in critical scenarios.

---

## Current Status (v0.1.0-alpha)

**Working / Verified on hardware:**
- Kyber-512 handshake via BLE
- X25519 key exchange
- End-to-end encrypted chat
- Single-hop onion routing
- Full Web UI
- Peer management with policies, whitelists, blocklists
- RTC persistence

**Not yet working:**
- Multi-hop onion routing
- WiFi STA discovery
- Deep sleep
- Voice (Codec2, in development)
- GPS, sensors (in development)

**Coming soon:**
- OTA firmware updates
- PUF, secure boot
- PCB production
- Hardware certifications

*Full details in STATUS.md and ROADMAP.md.*

---

## Media Assets

All logos, images, videos, screenshots, test footage, and promotional materials are available in the **`assets`** folder of the GitHub repository:

**https://github.com/qorlyn/qorlyn-firmware/tree/main/assets**

---

## Press Contact

**Email:** qorlyn@qorlyn.org

For security-related inquiries: security@qorlyn.org
For community conduct: conduct@qorlyn.org

---

## Useful Links

- **Website:** https://qorlyn.org
- **GitHub:** https://github.com/qorlyn/qorlyn-firmware
- **Manifesto:** https://qorlyn.org/manifesto
- **Documentation:** https://qorlyn.org/docs
- **Threat Model:** https://qorlyn.org/threat-model
- **Discord:** https://qorlyn.org/discord
- **Telegram, Facebook, Twitter, Instagram, TikTok, Mastodon, Matrix:** All under the handle **"Qorlyn"**

---

## Boilerplate

**50 words:**
Qorlyn is a decentralized, post-quantum, censorship-resistant mesh communication protocol. It works without internet, SIM cards, or servers. Every device is a node. Licensed under AGPL-3.0, it is designed for war zones, disasters, and regimes with high censorship.

**100 words:**
Qorlyn is a decentralized, post-quantum, censorship-resistant mesh communication protocol designed to operate without internet, SIM cards, servers, or permissions. Every device is an equal peer; the network forms where needed and dissolves when no longer required. Using Kyber-512 encryption, onion routing, and LoRa transport, Qorlyn enables secure communication in war zones, during natural disasters, and under oppressive regimes. The project is open source (AGPL-3.0), developed by a thirteen-year-old independent developer, and includes four device models: Vanilla, Lynx, Lynx Blackout, and BlackSil.

**200 words:**
Qorlyn is a decentralized, post-quantum, censorship-resistant mesh communication protocol. It operates entirely without internet, SIM cards, servers, or permissions. Every device is a node; every node is an equal peer. The network forms where it is needed and dissolves when no longer required.

Qorlyn uses Kyber-512, the NIST post-quantum standard, for encryption. Each session uses ephemeral keys destroyed after use. Onion routing ensures no single node sees both sender and recipient. LoRa transport provides up to 12 km range in open terrain, operating below the noise floor and resistant to jamming.

The project is open source under AGPL-3.0. Four device models are available: Qorlyn Vanilla (civilians, students, protesters), Qorlyn Lynx (activists, journalists, hikers), Qorlyn Lynx Blackout (journalists at risk, critical personnel), and Qorlyn BlackSil (war reporters, NGOs, private security).

Qorlyn was founded in April 2026 by Emanuele Ferraro, a thirteen-year-old independent developer from Italy, who believes freedom of expression is a fundamental human right. The project is currently in alpha (v0.1.0-alpha) and has passed over 70 tests on Heltec Meshtastic LoRa 32 v3.2 hardware.

---

## License

AGPL-3.0-or-later · Non-profit · Open source · Made in Italy

---

**QORLYN**
*The network that remains when the rest stops.*
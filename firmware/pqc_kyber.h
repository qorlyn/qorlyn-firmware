<!--
  ═══════════════════════════════════════════════════════════════
  QORLYN — Quantum Onion Routing Lattice lYnx Network
  Official README · v0.1.0-alpha
  ═══════════════════════════════════════════════════════════════
-->

<div align="center">

<!-- LOGO — replace with actual file when ready -->
<!-- <img src="assets/qorlyn-logo.svg" alt="Qorlyn" width="180"> -->

# QORLYN

### The network that remains when the rest stops.

**Quantum Onion Routing Lattice lYnx Network**

Decentralized, post-quantum, censorship-resistant mesh communication.
Born in the field. For those who have no alternative.

<br>

[![License](https://img.shields.io/badge/LICENSE-AGPL--3.0-00B8D4?style=for-the-badge&logo=gnu&logoColor=white)](LICENSE)
[![Status](https://img.shields.io/badge/STATUS-ALPHA-E8A800?style=for-the-badge)]()
[![Version](https://img.shields.io/badge/VERSION-0.1.0--alpha-0A0A0A?style=for-the-badge)]()
[![Platform](https://img.shields.io/badge/PLATFORM-ESP32--S3-2A7FA8?style=for-the-badge&logo=espressif&logoColor=white)]()
[![Radio](https://img.shields.io/badge/RADIO-LoRa%20868%20MHz-5C8B2F?style=for-the-badge)]()
[![PQC](https://img.shields.io/badge/PQC-KYBER--512-00B8D4?style=for-the-badge)]()
[![Onion](https://img.shields.io/badge/ONION-X25519%20PER--HOP-0F3A52?style=font-size:12px&style=for-the-badge)]()
[![Server](https://img.shields.io/badge/SERVER-ZERO-0A0A0A?style=for-the-badge)]()
[![Non Profit](https://img.shields.io/badge/NON--PROFIT-100%25-5C8B2F?style=for-the-badge)]()
[![PRs](https://img.shields.io/badge/PRs-WELCOME-00B8D4?style=for-the-badge)]()
[![Made in Italy](https://img.shields.io/badge/MADE%20IN-ITALY-0A0A0A?style=for-the-badge&logo=italy&logoColor=white)]()

</div>

---

## Index

**Part I — For Everyone**
1. [Manifesto](#1-manifesto)
2. [What Is Qorlyn](#2-what-is-qorlyn)
3. [Who It Is For](#3-who-it-is-for)
4. [What Makes It Different](#4-what-makes-it-different)
5. [The Device: Lynx](#5-the-device-lynx)
6. [Frequently Asked Questions](#6-frequently-asked-questions)

**Part II — Getting Started**
7. [What You Need](#7-what-you-need)
8. [Installation](#8-installation)
9. [First Boot](#9-first-boot)
10. [How To Use It](#10-how-to-use-it)

**Part III — Technical Documentation**
11. [Architecture Overview](#11-architecture-overview)
12. [Cryptographic Stack](#12-cryptographic-stack)
13. [Packet Format](#13-packet-format)
14. [Key Exchange](#14-key-exchange)
15. [Onion Routing](#15-onion-routing)
16. [Mesh Transport](#16-mesh-transport)
17. [Peer Management](#17-peer-management)
18. [Web Interface](#18-web-interface)
19. [Serial Console](#19-serial-console)
20. [Power Management](#20-power-management)

**Part IV — Project**
21. [Project Status](#21-project-status)
22. [Roadmap](#22-roadmap)
23. [Contributing](#23-contributing)
24. [Security Disclosure](#24-security-disclosure)
25. [License](#25-license)
26. [Credits](#26-credits)
27. [Contact](#27-contact)

---

# PART I — FOR EVERYONE

---

## 1. Manifesto

> We live in the future we were promised.
>
> Only, the promised future was **bright, open, free**.
> What we got is **surveilled, centralized, silent**.
>
> Censorship in 2026 is **outdated**. And yet it exists.
> Mass surveillance is **useless**. And yet it exists.
> Dependency on centralized infrastructure is **fragile**. And yet it exists.
>
> **Qorlyn is the technical answer to a human problem.**
>
> It is not nostalgia. It is **armed memory**.
> We remember what should have been, and we build what must be.
>
> *The future is not empty. It is full of information. The difference is clarity.*

---

## 2. What Is Qorlyn

**Qorlyn** is a **decentralized, anonymous, post-quantum communication protocol**, designed to work **where everything else shuts down**.

It is not a product. It is an **open standard**.

It does not depend on servers, operators, satellites, or centralized infrastructure. Every device is a node. Every node is a peer. The network forms where it is needed, and dissolves when it is not.

Qorlyn was born from a concrete need: **to allow journalists, activists, humanitarian workers, and civilians to communicate securely in conflict zones, under censorship, or in areas without infrastructure.**

> **Qorlyn is the network. Lynx is the device. The protocol belongs to everyone.**

### The name

**Qorlyn** stands for **Q**uantum **O**nion **R**outing **L**attice l**Y**nx **N**etwork.

- **Quantum** — the cryptographic foundation is post-quantum (Kyber-512).
- **Onion** — the routing model is layered, anonymous, Tor-like.
- **Routing** — the packets travel through multiple hops.
- **Lattice** — the mathematical structure behind the cryptography.
- **lYnx** — the lynx: silent, stealthy, precise. The animal behind the brand.
- **Network** — a mesh of peers, with no center.

Every letter has a reason. Nothing is random.

---

## 3. Who It Is For

Qorlyn is **not for everyone**. It is for those who have no alternative.

### Journalists and independent reporters
Communicate from conflict zones, under censorship, or in areas without coverage. Protect sources. Transmit material without traceability.

### Activists and human rights defenders
Coordinate during protests, in authoritarian contexts, or in areas with information blackouts. No SIM, no IMEI, no registry.

### NGOs and humanitarian operators
Coordinate teams in refugee camps, disaster zones, or remote areas. Local communication at near-zero cost, complementing expensive satellite radios.

### Civil protection and municipalities
Manage emergencies when cellular and electrical networks collapse due to natural disasters. Pre-configured kits, deployable in under 5 minutes.

### Government agencies and defense
Operational continuity in cyber-warfare scenarios. Tactical communication without fixed infrastructure, resistant to jamming and triangulation.

### Civil communities
Civic resilience. Anyone with a Qorlyn device can join the network, creating a mesh of free and independent communication.

---

## 4. What Makes It Different

There are many mesh communication projects. Qorlyn is different because of what it combines.

| Feature | Qorlyn | Typical Mesh | Satellite | Smartphone |
|---|---|---|---|---|
| **Post-quantum crypto** | Kyber-512 | No | Partial | No |
| **Onion routing** | X25519 per-hop | No | No | No |
| **Zero infrastructure** | Yes | Yes | No | No |
| **PUF key derivation** | Yes (planned) | No | No | No |
| **Physical wipe** | Yes (planned) | No | No | No |
| **Open source** | AGPL-3.0 | Varies | No | No |
| **No subscription** | Yes | Yes | No | No |
| **Cost** | ~€50-500 | €20-200 | €800-3000+ | €200+ |

The combination is what makes Qorlyn unique. No other project brings all of these together in a single, coherent, open, affordable device.

### What Qorlyn does NOT promise

We prefer to lose a reader than to mislead those who trust us.

- **It is not a satellite.** It does not send 4K video.
- **LoRa bandwidth is narrow**, by choice.
- **Absolute anonymity does not exist.** We make the attacker's job hard, not impossible.
- **Frequencies and power** depend on local regulations.
- **20W is legal only where permitted.**
- **Qorlyn is a tool, not a miracle.**

For the full picture, read the [Threat Model](docs/THREAT_MODEL.md).

---

## 5. The Device: Lynx

**Lynx** is the portable device for the end user. A low-power network computer, rugged, built to be used where no other device survives.

### Hardware specification

| Component | Detail |
|---|---|
| **MCU** | ESP32-S3 (dual core, WiFi + BLE) |
| **Radio** | Semtech SX1262 (LoRa) |
| **Display** | OLED 128×64 (I2C) |
| **Cryptography** | Kyber-512 + AES-256-GCM + X25519 |
| **Chassis** | Shockproof IK09 |
| **Environment** | IP67 water / dust resistant |
| **Temperature** | −20°C to +60°C |
| **Power** | Dual battery (internal + swap-in) |
| **Charging** | 20W solar panel + USB-C |
| **Sensors** | Temperature, gas, smoke, radiation (planned) |
| **Hardware security** | PUF, Secure Boot, physical wipe (planned) |
| **Connectivity** | LoRa mesh + WiFi/BT gateway (in development) |

### Versions

Three tiers, same firmware, same protocol, different hardware.

- **Qorlyn Vanilla** (~€50) — Entry level. Heltec V3.2 board, membrane keyboard, 3D-printed case. For civilians, students, protesters.
- **Qorlyn Lynx** (~€120) — Tested, sturdier. Custom PCB, larger battery, IP54 case. For activists, freelance journalists, hikers.
- **Qorlyn Lynx Blackout** (~€250) — For serious use. Removable GPS and mic, dual battery, IP67 case, emergency wipe. For journalists at risk, operators in critical areas.
- **Qorlyn BlackSil** (~€500) — The flagship. PUF, secure boot, full sensors, solar panel, physical wipe. For war correspondents, NGOs, private security.

*Same protocol. Same network. Different armor.*

---

## 6. Frequently Asked Questions

**Do I need internet?**
No. Qorlyn works without internet, without SIM, without any central infrastructure.

**Do I need a license?**
No. Frequencies used (868 MHz in EU, 915 MHz in US) are license-free ISM bands. Local regulations apply.

**Can the government track me?**
Qorlyn hides content and, in onion mode, hides metadata. But no system is perfect. Read the [Threat Model](docs/THREAT_MODEL.md) to understand the limits.

**How far does it reach?**
10-15 km per hop in open field with 1.5W. With relay nodes, the network extends further. Indoor or urban: 1-3 km.

**Can I talk on it?**
Voice is planned (Codec2) but not yet implemented. Currently: text only.

**How much does it cost?**
DIY kit: ~€40. Finished device: €50 to €500 depending on model.

**Is it legal?**
Yes, in most countries. Encryption is legal in most jurisdictions. Check your local laws. You are responsible for compliant use.

**Who maintains it?**
Currently a small team. The project is designed to be forkable and community-maintained.

**Can I contribute?**
Yes. See [Contributing](#23-contributing).

---

# PART II — GETTING STARTED

---

## 7. What You Need

### Minimum hardware (for testing)

- 2 × **Heltec LoRa 32 V3.2** (or compatible ESP32 + SX1262 board)
- 2 × USB-C cables
- A computer with **Arduino IDE 2.x** or **PlatformIO**

### Optional hardware (for the full setup)

- 1 × 868 MHz antenna (SMA, +3 dBi or better)
- 1 × 3.7V LiPo battery (2000 mAh or more)
- 1 × 3D-printed case (STL files in [`/case`](/case))
- 1 × solar panel (5V, 100 mA)

### Software requirements

- **Arduino IDE 2.x** or **PlatformIO** with ESP32 support
- **ESP32 core 3.x** (`esp32s3`)
- **RadioLib** ≥ 7.0
- **ESPAsyncWebServer** (ESP32Async fork)
- **AsyncTCP** (ESP32Async fork, ≥ 3.3.3)
- **Adafruit SSD1306** and **Adafruit GFX**
- **pqc_kyber** library (see [integration notes](#12-cryptographic-stack))

### Build flags

Create a `build_opt.h` file in the sketch folder with these flags:

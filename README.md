# <img src="assets/qorlyn-symbol.svg" width="32" alt="Qorlyn"> QORLYN

### La rete che resta quando il resto smette.

**Quantum Onion Routing Lattice lYnx Network**

Comunicazione mesh decentralizzata, post-quantistica, resistente alla censura.
Nata sul campo. Per chi non ha alternative.

<br>

[![Website](https://img.shields.io/badge/WEBSITE-qorlyn.org-00B8D4?style=for-the-badge&logo=firefox&logoColor=white)](https://qorlyn.org)
[![License](https://img.shields.io/badge/LICENSE-AGPL--3.0-5C8B2F?style=for-the-badge&logo=gnu&logoColor=white)](LICENSE)
[![Status](https://img.shields.io/badge/STATUS-ALPHA-E8A800?style=for-the-badge)](STATUS.md)
[![Version](https://img.shields.io/badge/VERSION-0.1.0--alpha-0A0A0A?style=for-the-badge)](CHANGELOG.md)

[![Platform](https://img.shields.io/badge/PLATFORM-ESP32--S3-2A7FA8?style=for-the-badge&logo=espressif&logoColor=white)](https://www.espressif.com/en/products/socs/esp32-s3)
[![Radio](https://img.shields.io/badge/RADIO-LoRa%20868%20MHz-5C8B2F?style=for-the-badge)](https://qorlyn.org/docs/radio)
[![PQC](https://img.shields.io/badge/PQC-KYBER--512-00B8D4?style=for-the-badge)](https://qorlyn.org/docs/crypto)
[![Onion](https://img.shields.io/badge/ONION-X25519%20PER--HOP-0F3A52?style=for-the-badge)](https://qorlyn.org/docs/onion)

[![Server](https://img.shields.io/badge/SERVER-ZERO-0A0A0A?style=for-the-badge)](https://qorlyn.org/manifesto)
[![Non Profit](https://img.shields.io/badge/NON--PROFIT-100%25-5C8B2F?style=for-the-badge)](https://qorlyn.org/manifesto)
[![Open Source](https://img.shields.io/badge/OPEN-SOURCE-00B8D4?style=for-the-badge)](https://github.com/qorlyn)
[![PRs](https://img.shields.io/badge/PRs-WELCOME-00B8D4?style=for-the-badge)](CONTRIBUTING.md)
[![Made in Italy](https://img.shields.io/badge/MADE%20IN-ITALY-0A0A0A?style=for-the-badge)](https://qorlyn.org)

<br>

[**Manifesto**](https://qorlyn.org/manifesto) · [**Cos'è**](#cosè-qorlyn) · [**Come funziona**](#come-funziona) · [**Dispositivi**](#dispositivi) · [**Download**](#download) · [**Contribuire**](CONTRIBUTING.md) · [**Contatti**](#contatti)

---

<img src="assets/banner.png" alt="Qorlyn — The network that remains when the rest stops" width="100%">

---

## Cos'è Qorlyn

C'è un momento preciso in cui capisci che il mondo che ti avevano promesso non esiste.

Sei in una zona di guerra, o sotto un regime che ha appena spento internet, o in un campo profughi dopo un terremoto. Le torri cellulari sono cadute. La SIM non prende. Il satellite costa tremila euro e ti traccia.

Hai bisogno di mandare un messaggio. Poche parole. Un pezzo, una coordinata, un "sono vivo".

Il tuo telefono è inutile. Le radio normali sono in chiaro, chiunque può ascoltare. Le app di messaggistica hanno bisogno di internet, e internet non c'è.

**Qorlyn esiste per quel momento.**

Qorlyn è un protocollo di comunicazione **decentralizzato, anonimo, post-quantistico**. Funziona senza internet, senza SIM, senza server, senza permessi. Ogni dispositivo è un nodo. Ogni nodo è pari. La rete si forma dove serve, si scioglie quando non serve più.

Non è un prodotto commerciale. È uno **standard aperto**, sotto licenza [AGPL-3.0](LICENSE). Il codice è pubblico. Chiunque può verificarlo, modificarlo, distribuirlo. Nessuno può chiuderlo, venderlo come proprietario, o metterci una backdoor.

> **Qorlyn è la rete. Lynx è il dispositivo. Il protocollo è di tutti.**

---

## Il problema

Nel 2026, la censura è **antiquata**. Eppure esiste.

Nel 2026, la sorveglianza di massa è **inutile**. Eppure esiste.

Nel 2026, la dipendenza da infrastrutture centralizzate è **fragile**. Eppure esiste.

**43 giornalisti** uccisi nel 2024, molti mentre cercavano di comunicare.
**12 paesi** con blackout informativo attivo.
**1.2 miliardi di persone** sotto sorveglianza di massa.

**Qorlyn è la risposta tecnica a un problema umano.**

---

## Come funziona

### Crittografia post-quantistica

Kyber-512, lo standard NIST per resistere ai futuri computer quantistici. Ogni sessione ha chiavi effimere, distrutte dopo l'uso. Anche se qualcuno registra il traffico oggi, non potrà decifrarlo domani.

### Onion routing

Ogni pacchetto attraversa tre nodi casuali prima di arrivare. Ogni hop conosce solo il precedente e il successivo. Nessun nodo vede insieme mittente e destinatario. Modello Tor, adattato alla radio.

### Trasporto LoRa

Modulazione a spettro espanso, banda ISM libera (868 MHz in Europa). Fino a 12 km in campo aperto con 1.5W. Funziona sotto la soglia del rumore. Resistente al jamming per natura della modulazione.

[**Approfondisci l'architettura →**](https://qorlyn.org/docs/architecture)

---

## Il wipe fisico

<img src="assets/wipe-diagram.svg" alt="Wipe fisico: distruzione del chip in caso di sequestro" width="100%">

Se ti prendono, se ti sequestrano il dispositivo, se provano a estrarre le chiavi con la forza — **tiri una corda**. Il chip si distrugge fisicamente. Non solo le chiavi. Il processore, la memoria, tutto. In meno di due secondi.

Non c'è niente da recuperare, perché non c'è più niente.

Gli HSM lo fanno sui server. I telefoni hanno il "cancella dati". Nessun dispositivo portatile commerciale ha una corda che rompe il silicio.

---

## Dispositivi

Quattro versioni, stesso protocollo. Un Vanilla e un BlackSil parlano la stessa lingua. Stessa cifratura. Stessa rete.

<table>
<tr>
<td align="center" width="25%">
<img src="assets/device-vanilla.png" alt="Qorlyn Vanilla"><br>
<b>Qorlyn Vanilla</b><br>
<sub>Il punto d'ingresso</sub>
</td>
<td align="center" width="25%">
<img src="assets/device-lynx.png" alt="Qorlyn Lynx"><br>
<b>Qorlyn Lynx</b><br>
<sub>Testato, più robusto</sub>
</td>
<td align="center" width="25%">
<img src="assets/device-blackout.png" alt="Qorlyn Lynx Blackout"><br>
<b>Qorlyn Lynx Blackout</b><br>
<sub>Per chi fa sul serio</sub>
</td>
<td align="center" width="25%">
<img src="assets/device-blacksil.png" alt="Qorlyn BlackSil"><br>
<b>Qorlyn BlackSil</b><br>
<sub>Il flagship</sub>
</td>
</tr>
</table>

**Qorlyn Vanilla** — Board Heltec V3.2, tastiera a membrana, case stampato in 3D. Fa tutto quello che serve per comunicare. Per civili, studenti, manifestanti.

**Qorlyn Lynx** — PCB custom, batteria più grande, case IP54. Per attivisti, giornalisti freelance, escursionisti.

**Qorlyn Lynx Blackout** — GPS e microfono rimovibili fisicamente, doppia batteria, case IP67, wipe di emergenza. Per giornalisti a rischio, operatori in aree critiche.

**Qorlyn BlackSil** — PUF, secure boot, sensori completi, pannello solare integrato, wipe fisico. Per reporter di guerra, ONG, sicurezza privata.

[**Specifiche complete →**](https://qorlyn.org/devices)

---

## Cosa Qorlyn NON promette

Preferiamo perdere un lettore che illudere chi si fida di noi.

- **Non è un satellite.** Non manda video 4K. Non chiama dall'altra parte del mondo.
- **La banda LoRa è stretta.** 20 kilobit al secondo. Testo, coordinate, SOS.
- **L'anonimato assoluto non esiste.** Rendiamo il lavoro dell'attaccante difficile, non impossibile.
- **L'onion routing nasconde il destinatario, non il mittente.** Il primo hop vede chi ha mandato il messaggio.
- **Le frequenze dipendono dalle normative locali.** 868 MHz in Europa, 915 MHz negli USA.
- **Qorlyn è uno strumento, non un miracolo.**

La [threat model](https://qorlyn.org/threat-model) completa è pubblica. Leggila prima di usare Qorlyn in scenari critici.

---

## Download

### Firmware

| Canale | Versione | Link |
|---|---|---|
| **Stabile** | v0.1.0-alpha | [Download](https://github.com/qorlyn/firmware/releases/latest) |
| **Sviluppo** | main branch | [Clona il repo](https://github.com/qorlyn/firmware) |
| **Archivio** | Tutte le release | [Releases](https://github.com/qorlyn/firmware/releases) |

### Da cosa iniziare

1. **Leggi la [guida all'installazione](https://qorlyn.org/docs/install)**
2. **Controlla i [requisiti hardware](https://qorlyn.org/docs/hardware)**
3. **Segui il [primo avvio](https://qorlyn.org/docs/first-boot)**
4. **Unisciti al [Discord](https://qorlyn.org/discord)** per supporto

### File del progetto

- [`CHANGELOG.md`](CHANGELOG.md) — Storia delle versioni
- [`STATUS.md`](STATUS.md) — Stato attuale e bug noti
- [`ROADMAP.md`](ROADMAP.md) — Cosa arriverà
- [`CONTRIBUTING.md`](CONTRIBUTING.md) — Come contribuire
- [`CODE_OF_CONDUCT.md`](CODE_OF_CONDUCT.md) — Regole della community
- [`LICENSE`](LICENSE) — AGPL-3.0

---

## Stato attuale

**Versione:** v0.1.0-alpha

**Funziona, verificato su hardware:**
- Handshake Kyber-512 via BLE
- Scambio chiavi X25519
- Chat cifrata end-to-end
- Onion routing single-hop
- Web UI completa
- Gestione peer con policy, whitelist, blocklist
- Persistenza RTC

**Non funziona ancora:**
- Onion multi-hop (serve un terzo board)
- WiFi STA discovery (problema di stack)
- Deep sleep (codice c'è, non testato)
- Voce (Codec2, in sviluppo)
- GPS, sensori (in sviluppo)

**Arriverà:**
- OTA firmware update
- PUF, secure boot
- Produzione PCB
- Certificazione CE/FCC
- Primo lotto via Crowd Supply

Dettagli completi in [`STATUS.md`](STATUS.md) e [`ROADMAP.md`](ROADMAP.md).

---

## Contribuire

Qorlyn è open source sotto [AGPL-3.0](LICENSE). Chiunque può contribuire: bug fix, test su hardware, documentazione, traduzioni, audit di sicurezza.

**Cosa cerchiamo:**
- Bug fix nel firmware
- Test su hardware reale (obbligatorio per ogni PR)
- Traduzioni (inglese, spagnolo, arabo, farsi, ucraino, russo)
- Audit del codice crittografico

**Cosa non cerchiamo:**
- Riscritture complete senza discussione
- Codice AI non testato e non compreso
- Backdoor, telemetria, tracking

Linee guida complete in [`CONTRIBUTING.md`](CONTRIBUTING.md).

---

## Domande frequenti

**Serve internet?** No. Mai.
**Serve una SIM?** No. Mai.
**Serve un server?** No. Non esiste un server Qorlyn.
**È legale?** Sì, nella maggior parte dei paesi. Verifica le leggi locali.
**Posso essere tracciato?** Il contenuto è cifrato. I metadati sono parzialmente nascosti dall'onion routing.
**Quanto arriva lontano?** 10-15 km per hop in campo aperto. 1-3 km in città.
**Posso parlare?** Non ancora. Voce in sviluppo (Codec2).
**Chi c'è dietro?** Un piccolo team. Il progetto è progettato per essere forkabile e mantenuto dalla community.

[**Altre domande →**](https://qorlyn.org/faq)

---

## Contatti

[![Website](https://img.shields.io/badge/Website-qorlyn.org-00B8D4?style=flat-square)](https://qorlyn.org)
[![Email](https://img.shields.io/badge/Email-qorlyn@qorlyn.org-5C8B2F?style=flat-square)](mailto:qorlyn@qorlyn.org)
[![Discord](https://img.shields.io/badge/Discord-qorlyn.org%2Fdiscord-2A7FA8?style=flat-square)](https://qorlyn.org/discord)
[![Telegram](https://img.shields.io/badge/Telegram-qorlyn.org%2Ftelegram-4A9ECD?style=flat-square)](https://qorlyn.org/telegram)
[![GitHub](https://img.shields.io/badge/GitHub-github.com%2Fqorlyn-0A0A0A?style=flat-square)](https://github.com/qorlyn)
[![Forum](https://img.shields.io/badge/Forum-forum.qorlyn.org-5C8B2F?style=flat-square)](https://forum.qorlyn.org)

**Canali dedicati**
- **Sicurezza:** [security@qorlyn.org](mailto:security@qorlyn.org) — [policy completa](https://qorlyn.org/security)
- **Condotta:** [conduct@qorlyn.org](mailto:conduct@qorlyn.org) — [code of conduct](CODE_OF_CONDUCT.md)
- **Stampa:** [press@qorlyn.org](mailto:press@qorlyn.org)

**Sostieni il progetto**
- [Donazioni](https://qorlyn.org/donate)
- [Sponsor](https://github.com/sponsors/qorlyn)

---

<div align="center">

<img src="assets/qorlyn-symbol.svg" width="48" alt="Qorlyn">

**QORLYN**

*La rete che resta quando il resto smette.*

<sub>AGPL-3.0-or-later · Non-profit · Open source · Made in Italy</sub>

</div>
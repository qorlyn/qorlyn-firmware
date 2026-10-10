# CONTRIBUTING.md

# Contributing to Qorlyn

Thank you for your interest. Qorlyn is an open source project under the AGPL-3.0-or-later license. Every contribution is welcome, but there are rules.

Before opening an issue or a pull request, read this document. It will save you time, and it will save time for the maintainers.

## Table of Contents

1. Code of Conduct
2. What We Look For
3. What We Do Not Look For
4. Reporting a Bug
5. Proposing a Feature
6. Submitting a Pull Request
7. Code Style
8. Development Environment Setup
9. Reporting a Vulnerability
10. License and Copyright

## Code of Conduct

By participating in this project you agree to the Code of Conduct. Violations may lead to exclusion from the project.

## What We Look For

Qorlyn is a technical project with a precise mission: secure, anonymous, censorship-resistant communication for those who need it.

The most useful contributions are:

- Bug fixes in the firmware (transport, cryptography, peer management)
- Testing on real hardware (Heltec V3.2, other ESP32+SX1262 boards)
- Documentation (README, code comments, guides)
- Translations (English, Spanish, Arabic, Farsi, Ukrainian, Russian)
- Security audits of the cryptographic code
- Optimizations for power consumption, RAM, latency
- New transports (ESP-NOW, 802.11s, others)

If you are unsure whether a contribution is useful, open an issue and ask first.

## What We Do Not Look For

- Complete rewrites without prior discussion
- New features not discussed in an issue first
- Style changes without a technical reason
- Adding dependencies without justification
- AI-generated code that is not tested and not understood
- Changes that violate the AGPL-3.0 license
- Backdoors, telemetry, or any form of tracking

If you are not sure, ask first. It is better to lose 10 minutes in an issue than 10 hours in a rejected pull request.

## Reporting a Bug

Before opening an issue, search existing issues. It may already be reported.

If it is not, open an issue with the following information:

- Description of the bug: what happens, clearly.
- How to reproduce it: step 1, step 2, step 3.
- Expected behavior: what should happen.
- Observed behavior: what actually happens.
- Hardware: board model, firmware version, other relevant info.
- Logs: paste serial logs if available.
- Additional notes: any other useful information.

Do not open issues for general questions (use Discord), personal support requests (use Discord), or philosophical discussions about the project (use Discord).

## Proposing a Feature

Open an issue with the enhancement label and describe:

1. The problem you want to solve (not the solution).
2. Who benefits (journalists? activists? NGOs?).
3. How you would solve it (if you have an idea).
4. Alternatives considered.

A feature that does not solve a real problem will not be accepted. Qorlyn is not a project for doing cool things. It is a project for solving concrete problems of concrete people.

## Submitting a Pull Request

Before writing code, open an issue. This avoids you working for 3 days on something that will not be accepted.

Procedure:

1. Fork the repository.
2. Create a branch with a descriptive name, such as fix/onion-ack-timeout, feat/wifi-sta-discovery, or docs/threat-model-update.
3. Write the code.
4. Test on real hardware. Untested pull requests will not be accepted.
5. Update documentation if necessary.
6. Open the pull request against main.

Pull request template:

- What this PR does: brief description.
- How it was tested: hardware used, scenario, result.
- Linked issue: closes #XX.
- Breaking changes: yes or no. If yes, explain.
- Notes for the reviewer: anything the reviewer should know.

What to expect:

- The pull request will be reviewed by a maintainer.
- Changes may be requested.
- It may take days before a response.
- Not all pull requests are accepted.

Be patient. Be willing to discuss. Be ready to redo.

## Code Style

Qorlyn uses C++ (Arduino/ESP-IDF). Follow these rules:

Formatting:
- Indentation: 4 spaces (no tabs).
- Lines: maximum 100 characters.
- Braces: K&R style (opening brace on the same line).
- Spaces: after commas, around operators.

Naming:
- Global variables: g_camelCase (example: g_sessionCounter).
- Local variables: camelCase (example: sessionCounter).
- Constants: UPPER_SNAKE_CASE (example: MAX_PEERS).
- Functions: camelCase (example: handleOnionLayer).
- Types: PascalCase (example: PeerSession).
- Enums: PascalCase for the type, UPPER_SNAKE_CASE for values.

Comments:
- Yes: explain why, not what.
- No: obvious comments.
- Yes: comments on design decisions.
- No: commented-out code (use Git).

Security:
- Never use strcpy, sprintf, gets. Use strncpy, snprintf, fgets.
- Never use buffers without length checks.
- Never hardcode keys in the code.
- Always check return values from mbedtls.
- Always zero keys from memory after use.

Commits:
Format: type: short description.

Accepted types:
- feat: new feature
- fix: bug fix
- docs: documentation
- refactor: rewrite without functional changes
- test: testing
- chore: maintenance

Example: feat: add WiFi STA discovery with retry logic

The commit body explains why, not what (the diff already says that).

## Development Environment Setup

Requirements:
- Arduino IDE 2.x or PlatformIO
- ESP32 core 3.x (esp32s3)
- RadioLib 7.x
- ESPAsyncWebServer (ESP32Async fork)
- AsyncTCP (ESP32Async fork, version 3.3.3 or later)
- Adafruit SSD1306 and Adafruit GFX
- pqc_kyber (manual installation)

Build:
1. Clone the repository.
2. Open firmware/firmware.ino (or main.cpp) in the IDE.
3. Ensure build_opt.h contains the required defines. Refer to the README for the exact flags.
4. Select board: ESP32S3 Dev Module.
5. Compile.

Flash:
1. Connect the Heltec V3.2 via USB-C.
2. Select the correct serial port.
3. Click Upload.
4. Open the Serial Monitor at 115200 baud.

Testing:
There is no automated test suite. Tests are manual, on real hardware. See docs/TEST_PLAN.md for the procedure.

## Reporting a Vulnerability

Do not open a public issue for security vulnerabilities.

Send an email to security@qorlyn.org.

Include:
- Description of the vulnerability.
- How to reproduce it.
- Potential impact.
- Proposed fix if available.

If you wish, encrypt the email with the PGP key published in the repository.

There is no bug bounty. But if the vulnerability is critical and confirmed, your name will be added to the credits if you want.

## License and Copyright

Qorlyn is distributed under AGPL-3.0-or-later.

By contributing, you agree that your code will be distributed under the same license.

No CLA is required. But you must have the right to contribute the code you submit. If the code belongs to your employer, make sure you have permission.

Every source file must have the AGPL-3.0 header with your name (or pseudonym) as author.

## Questions?

Open an issue, or join the Discord at qorlyn.org/discord.

Thank you for contributing.

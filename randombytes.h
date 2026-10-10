/* ============================================================================
 * Qorlyn Firmware - https://github.com/qorlyn/qorlyn-firmware
 * Copyright (C) 2026 Emanuele Ferraro (github.com/emanueleferraro), Qorlyn (github.com/qorlyn).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * ----------------------------------------------------------------------------
 * ML-KEM (Kyber-512) + AES-256-GCM + Onion routing (X25519 effimero per-hop)
 * + Multi-protocol transport (LoRa / WiFi STA+AP / BLE peripheral+central)
 * ----------------------------------------------------------------------------
 * Connect with us:
 *   🌐 Website:  https://qorlyn.org
 *   📧 Email:    qorlyn@qorlyn.org
 *   💬 Forum:    https://forum.qorlyn.org
 *    @ Discord:  qorlyn.org/discord
 *    @ Telegram: qorlyn.org/telegram
 *    @ GitHub:   qorlyn.org/github
 *    Donate:     qorlyn.org/donate
 * ==========================================================================*/

#ifndef RANDOMBYTES_H
#define RANDOMBYTES_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <esp_random.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int randombytes(uint8_t *output, size_t n) {
    uint32_t r;
    size_t i;
    
    for (i = 0; i + sizeof(uint32_t) <= n; i += sizeof(uint32_t)) {
        r = esp_random();
        memcpy(output + i, &r, sizeof(uint32_t));
    }
    
    if (i < n) {
        r = esp_random();
        memcpy(output + i, &r, n - i);
    }
    return 0;
}

#ifdef __cplusplus
}
#endif

#endif

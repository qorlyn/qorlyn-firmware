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
 * Connect with us:
 *   🌐 Website:  https://qorlyn.org
 *   📧 Email:    qorlyn@qorlyn.org
 *   💬 Forum:    https://forum.qorlyn.org
 *    @ Discord:  qorlyn.org/discord
 *    @ Telegram: qorlyn.org/telegram
 *    @ GitHub:   qorlyn.org/github
 *    Donate:     qorlyn.org/donate
 * ==========================================================================*/

#ifndef PQC_KYBER_H
#define PQC_KYBER_H

#ifdef __cplusplus
extern "C" {
#endif

// Includi randombytes.h per la generazione casuale
#include "randombytes.h"

// Includi l'API di ML-KEM dal core PQClean
#include "kem.h"

// Mappiamo i nomi che usa il tuo codice G-Mesh a quelli REALI di ML-KEM
// Nota: la tua implementazione usa ML-KEM-512, equivalente a Kyber-512
#define pqc_kyber512_keypair       PQCLEAN_MLKEM512_CLEAN_crypto_kem_keypair
#define pqc_kyber512_encapsulate   PQCLEAN_MLKEM512_CLEAN_crypto_kem_enc
#define pqc_kyber512_decapsulate   PQCLEAN_MLKEM512_CLEAN_crypto_kem_dec

// Dimensioni per ML-KEM-512 (da params.h)
#define KYBER_PUBLICKEYBYTES       800
#define KYBER_SECRETKEYBYTES       1632
#define KYBER_CIPHERTEXTBYTES      768
#define KYBER_SSBYTES              32

#ifdef __cplusplus
}
#endif

#endif

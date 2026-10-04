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
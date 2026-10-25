#ifndef PQC_KYBER_H
#define PQC_KYBER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "randombytes.h"

#include "kem.h"

#define pqc_kyber512_keypair       PQCLEAN_MLKEM512_CLEAN_crypto_kem_keypair
#define pqc_kyber512_encapsulate   PQCLEAN_MLKEM512_CLEAN_crypto_kem_enc
#define pqc_kyber512_decapsulate   PQCLEAN_MLKEM512_CLEAN_crypto_kem_dec

#define KYBER_PUBLICKEYBYTES       800
#define KYBER_SECRETKEYBYTES       1632
#define KYBER_CIPHERTEXTBYTES      768
#define KYBER_SSBYTES              32

#ifdef __cplusplus
}
#endif

#endif

// randombytes.h - Versione ESP32 (SOSTITUISCI QUELLO CHE HAI)
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
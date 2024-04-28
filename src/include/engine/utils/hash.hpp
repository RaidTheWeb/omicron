#ifndef _ENGINE__UTILS__HASH_HPP
#define _ENGINE__UTILS__HASH_HPP

#include <engine/assertion.hpp>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

namespace OUtils {

#define FNV1A_PRIME 0x01000193
#define FNV1A_SEED 0x811C9DC5

    static inline uint32_t fnv1a(uint8_t byte, uint32_t hash = FNV1A_SEED) {
        return (byte ^ hash) * FNV1A_PRIME;
    }

    static inline uint32_t fnv1a(uint16_t byte16, uint32_t hash = FNV1A_SEED) {
        const uint8_t *ptr = (uint8_t *)&byte16;
        hash = fnv1a(*ptr++, hash);
        return fnv1a(*ptr, hash);
    }

    static inline uint32_t fnv1a(uint32_t byte32, uint32_t hash = FNV1A_SEED) {
        const uint8_t *ptr = (uint8_t *)&byte32;
        hash = fnv1a(*ptr++, hash);
        hash = fnv1a(*ptr++, hash);
        hash = fnv1a(*ptr++, hash);
        return fnv1a(*ptr, hash);
    }

    static inline uint32_t fnv1a(uint64_t byte64, uint32_t hash = FNV1A_SEED) {
        const uint8_t *ptr = (uint8_t *)&byte64;
        hash = fnv1a(*ptr++, hash);
        hash = fnv1a(*ptr++, hash);
        hash = fnv1a(*ptr++, hash);
        hash = fnv1a(*ptr++, hash);
        hash = fnv1a(*ptr++, hash);
        hash = fnv1a(*ptr++, hash);
        hash = fnv1a(*ptr++, hash);
        return fnv1a(*ptr, hash);
    }

    static inline uint32_t fnv1a(void *ptr, uint32_t hash = FNV1A_SEED) {
        return fnv1a((uint64_t)ptr, hash);
    }

    static inline uint32_t fnv1a(const void *data, size_t len, uint32_t hash = FNV1A_SEED) {
        ASSERT(data != NULL, "Data is NULL.\n");
        const uint8_t *ptr = (uint8_t *)data;
        while (len--) {
            hash = fnv1a(*ptr++, hash);
        }
        return hash;
    }

    static inline uint32_t fnv1a(const char *str, uint32_t hash = FNV1A_SEED) {
        return fnv1a(str, strlen(str), hash);
    }

    constexpr uint32_t STRINGID(const char *str, uint32_t hash = FNV1A_SEED) {
        return *str != '\0' ? STRINGID(str + 1, (*str ^ hash) * FNV1A_PRIME) : hash;
    }

}

#endif

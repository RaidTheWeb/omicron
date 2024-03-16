#ifndef _ENGINE__UTILS_HPP
#define _ENGINE__UTILS_HPP

#include <engine/assertion.hpp>
#include <engine/ext/stb_ds.hpp>
#include <engine/ext/stb_image.hpp>
#include <sys/time.h>
#include <stdio.h>
#include <stdint.h>

// defined for API export from engine executable (exposes the function to scripts)
#define OMICRON_EXPORT

#define FNV_PRIME 16777619
#define FNV_OFFBIAS 2166136261U

static inline const uint32_t utils_stringid(const char *str) {
    uint32_t hash = FNV_OFFBIAS;
    const char *ptr = (str);
    while (*ptr != '\0') {
        hash ^= (uint32_t)(*ptr++);
        hash *= FNV_PRIME;
    }
    return hash;
}

static inline char *utils_stringlifetime(const char *str) {
    char *e = (char *)malloc(strnlen(str, 64) + 1);
    ASSERT(e != NULL, "Failed to allocate memory for temporary lifetime string.\n");
    memset(e, 0, strnlen(str, 64) + 1);
    strncpy(e, str, strnlen(str, 64));
    return e;
}

struct utils_murmur2a {
    uint32_t hash;
    uint32_t tail;
    uint32_t count;
    uint32_t size;
};

static inline bool utils_isaligned(uint64_t a, int32_t align) {
    uint64_t mask = (uint64_t)(align - 1);
    return !(a & mask);
}

static inline void utils_murmur2abegin(struct utils_murmur2a *murmur, uint32_t seed) {
    murmur->hash = seed;
    murmur->tail = 0;
    murmur->count = 0;
    murmur->size = 0;
}

static inline uint32_t utils_readunaligned(uint8_t *data) {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return 0 | data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
#else
    return 0 | data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24;
#endif
}

#define mmix(h,k) { k *= 0x5bd1e995; k ^= k >> 24; k *= 0x5bd1e995; h *= 0x5bd1e995; h ^= k; }

static inline void utils_murmur2amixtail(struct utils_murmur2a *murmur, uint8_t *data, int32_t len) {
    while (len && ((len < 4) || murmur->count)) {
        murmur->tail |= (*data++) << (murmur->count * 8);

        murmur->count++;
        len--;

        if (murmur->count == 4) {
            mmix(murmur->hash, murmur->tail);
            murmur->tail = 0;
            murmur->count = 0;
        }
    }
}

static inline void utils_murmur2aadd(struct utils_murmur2a *murmur, const void *data, int32_t len) {
    uint8_t *pdata = (uint8_t *)data;

    murmur->size += len;
    if (__builtin_expect(!utils_isaligned((uint64_t)data, 4), 0)) {
        while (len >= 4) {
            uint32_t kk = utils_readunaligned((uint8_t *)data);
            mmix(murmur->hash, kk);

            pdata += 4;
            len -= 4;
        }
        utils_murmur2amixtail(murmur, pdata, len);

        return;
    }

    while (len >= 4) {
        uint32_t kk = *((uint32_t *)pdata);

        mmix(murmur->hash, kk);
        pdata += 4;
        len -= 4;
    }
}

static inline uint32_t utils_murmur2aend(struct utils_murmur2a *murmur) {
    mmix(murmur->hash, murmur->tail);
    mmix(murmur->hash, murmur->size);

    murmur->hash ^= murmur->hash >> 13;
    murmur->hash *= 0x5bd1e995;
    murmur->hash ^= murmur->hash >> 15;
    return murmur->hash;
}

static inline uint32_t utils_stridealignment(uint32_t offset, uint32_t align) {
    return offset + ((0 & -(offset % align == 0)) | ((align - (offset % align)) & ~(-((offset % align) == 0))));
}

struct utils_reader {
    size_t idx;
    uint8_t *data;
    size_t size;
};

static inline void utils_readerinit(struct utils_reader *reader, uint8_t *data, size_t size) {
    reader->idx = 0;
    reader->data = data;
    reader->size = size;
}

static inline bool utils_readerbyte8(struct utils_reader *reader, uint8_t *byte) {
    if (reader->idx >= reader->size) {
        return false; // can't read further than we have data for
    }

    *byte = *((uint8_t *)(reader->data + reader->idx));
    reader->idx += sizeof(uint8_t);
    return true;
}

static inline bool utils_readerbyte16(struct utils_reader *reader, uint16_t *byte16) {
    if (reader->idx >= reader->size || reader->idx + sizeof(uint16_t) > reader->size) {
        return false; // can't reader further
    }

    *byte16 = *((uint16_t *)(reader->data + reader->idx));
    reader->idx += sizeof(uint16_t);
    return true;
}

static inline bool utils_readerbyte32(struct utils_reader *reader, uint32_t *byte32) {
    if (reader->idx >= reader->size || reader->idx + sizeof(uint32_t) > reader->size) {
        return false;
    }

    *byte32 = *((uint32_t *)(reader->data + reader->idx));
    reader->idx += sizeof(uint32_t);
    return true;
}

static inline bool utils_readerbyte64(struct utils_reader *reader, uint64_t *byte64) {
    if (reader->idx >= reader->size || reader->idx + sizeof(uint64_t) > reader->size) {
        return false;
    }

    *byte64 = *((uint64_t *)(reader->data + reader->idx));
    reader->idx += sizeof(uint64_t);
    return true;
}

static inline int utils_gcd(int a, int b) {
    if (!b) {
        return a;
    }

    return utils_gcd(b, a % b);
}

static inline bool utils_endswith(const char *str, const char *ending) {
    size_t len = strlen(str);
    size_t endlen = strlen(ending);
    if (len >= endlen) {
        return (!strncmp(str + (len - endlen), ending, endlen));
    } else {
        return false;
    }
}

static inline int64_t utils_getcounter(void) {
#if BX_CRT_NONE
	int64_t i64 = crt0::getHPCounter();
#elif BX_PLATFORM_WINDOWS || BX_PLATFORM_XBOXONE || BX_PLATFORM_WINRT
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	int64_t i64 = li.QuadPart;
#elif BX_PLATFORM_ANDROID
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	int64_t i64 = now.tv_sec * INT64_C(1000000000) + now.tv_nsec;
#elif BX_PLATFORM_EMSCRIPTEN
	int64_t i64 = int64_t(1000.0f * emscripten_get_now() );
#elif !BX_PLATFORM_NONE
	struct timeval now;
	gettimeofday(&now, 0);
	int64_t i64 = now.tv_sec*INT64_C(1000000) + now.tv_usec;
#else
	int64_t i64 = UINT64_MAX;
#endif // BX_PLATFORM_
	return i64;
}

OMICRON_EXPORT void *utils_downsize(void *data, int size, int bpp);
OMICRON_EXPORT void *utils_generatemips(void *fullsize, int size, int bpp, size_t *length);
// OMICRON_EXPORT bgfx_shader_handle_t utils_loadshader(const char *filename);
// OMICRON_EXPORT bgfx_texture_handle_t utils_loadtexture(const char *filename, bool clamp);
// OMICRON_EXPORT bgfx_texture_handle_t utils_loadcubemapfromsides(const char *sides[], bool mips);
// OMICRON_EXPORT bgfx_texture_handle_t utils_loadcubemapfromktx(const char *filename, bool mips);
OMICRON_EXPORT bool utils_checkktxpath(const char *filename);

#define VECTOR_INVALID_INDEX (-1)

#define VECTOR_INIT {0}

#define VECTOR_TYPE(TYPE) \
    struct { \
        TYPE *data; \
        size_t length; \
        size_t capacity; \
    }

#define VECTOR_ENSURE_LENGTH(VEC, LENGTH) do { \
    auto VECTOR_ENSURE_LENGTH_vec = VEC; \
    if ((LENGTH) >= VECTOR_ENSURE_LENGTH_vec->capacity) { \
        if (VECTOR_ENSURE_LENGTH_vec->capacity == 0) { \
            VECTOR_ENSURE_LENGTH_vec->capacity = 8; \
        } else { \
            VECTOR_ENSURE_LENGTH_vec->capacity *= 2; \
        } \
        VECTOR_ENSURE_LENGTH_vec->data = realloc(VECTOR_ENSURE_LENGTH_vec->data, \
            VECTOR_ENSURE_LENGTH_vec->capacity * sizeof(*VECTOR_ENSURE_LENGTH_vec->data)); \
    } \
} while (0)

#define VECTOR_PUSH_BACK(VEC, VALUE) ({ \
    auto VECTOR_PUSH_BACK_vec = VEC; \
    VECTOR_ENSURE_LENGTH(VEC, VECTOR_PUSH_BACK_vec->length); \
    VECTOR_PUSH_BACK_vec->data[VECTOR_PUSH_BACK_vec->length++] = VALUE; \
    VECTOR_PUSH_BACK_vec->length - 1; \
})

#define VECTOR_PUSH_FRONT(VEC, VALUE) VECTOR_INSERT(VEC, 0, VALUE)

#define VECTOR_INSERT(VEC, IDX, VALUE) do { \
    auto VECTOR_INSERT_vec = VEC; \
    size_t VECTOR_INSERT_index = IDX; \
    VECTOR_ENSURE_LENGTH(VEC, VECTOR_INSERT_vec->length); \
    for (size_t VECTOR_INSERT_i = VECTOR_INSERT_vec->length; VECTOR_INSERT_i > VECTOR_INSERT_index; VECTOR_INSERT_i--) { \
        VECTOR_INSERT_vec->data[VECTOR_INSERT_i] = VECTOR_INSERT_vec->data[VECTOR_INSERT_i - 1]; \
    } \
    VECTOR_INSERT_vec->length++; \
    VECTOR_INSERT_vec->data[VECTOR_INSERT_index] = VALUE; \
} while (0)

#define VECTOR_REMOVE(VEC, IDX) do { \
    auto VECTOR_REMOVE_vec = VEC; \
    for (size_t VECTOR_REMOVE_i = (IDX); VECTOR_REMOVE_i < VECTOR_REMOVE_vec->length - 1; VECTOR_REMOVE_i++) { \
        VECTOR_REMOVE_vec->data[VECTOR_REMOVE_i] = VECTOR_REMOVE_vec->data[VECTOR_REMOVE_i + 1]; \
    } \
    VECTOR_REMOVE_vec->length--; \
} while (0)

#define VECTOR_ITEM(VEC, IDX) ({ \
    size_t VECTOR_ITEM_idx = IDX; \
    auto VECTOR_ITEM_vec = VEC; \
    auto VECTOR_ITEM_result = (typeof(*VECTOR_ITEM_vec->data))VECTOR_INVALID_INDEX; \
    if (VECTOR_ITEM_idx < VECTOR_ITEM_vec->length) { \
        VECTOR_ITEM_result = VECTOR_ITEM_vec->data[VECTOR_ITEM_idx]; \
    } \
    VECTOR_ITEM_result; \
})

#define VECTOR_FIND(VEC, VALUE) ({ \
    auto VECTOR_FIND_vec = VEC; \
    ssize_t VECTOR_FIND_result = VECTOR_INVALID_INDEX; \
    for (size_t VECTOR_FIND_i = 0; VECTOR_FIND_i < VECTOR_FIND_vec->length; VECTOR_FIND_i++) { \
        if (VECTOR_FIND_vec->data[VECTOR_FIND_i] == (VALUE)) { \
            VECTOR_FIND_result = VECTOR_FIND_i; \
            break; \
        } \
    } \
    VECTOR_FIND_result; \
})

#define VECTOR_REMOVE_BY_VALUE(VEC, VALUE) do { \
    auto VECTOR_REMOVE_BY_VALUE_vec = VEC; \
    auto VECTOR_REMOVE_BY_VALUE_v = VALUE; \
    size_t VECTOR_REMOVE_BY_VALUE_i = VECTOR_FIND(VECTOR_REMOVE_BY_VALUE_vec, VECTOR_REMOVE_BY_VALUE_v); \
    VECTOR_REMOVE(VECTOR_REMOVE_BY_VALUE_vec, VECTOR_REMOVE_BY_VALUE_i); \
} while (0)

#define VECTOR_FOR_EACH(VEC, BINDING, ...) do { \
    auto VECTOR_FOR_EACH_vec = VEC; \
    for (size_t VECTOR_FOR_EACH_i = 0; VECTOR_FOR_EACH_i < VECTOR_FOR_EACH_vec->length; VECTOR_FOR_EACH_i++) { \
        auto BINDING = &VECTOR_FOR_EACH_vec->data[VECTOR_FOR_EACH_i]; \
        __VA_ARGS__ \
    } \
} while (0)

#endif

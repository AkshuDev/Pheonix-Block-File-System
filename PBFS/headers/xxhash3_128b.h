#pragma once

#include <stdint.h>
#include <stddef.h>

typedef uint64_t XXH64_hash_t;
typedef struct {
    XXH64_hash_t low64;
    XXH64_hash_t high64;
} XXH128_hash_t;

#ifdef __cplusplus
extern "C" {
#endif
#define XXH3_SECRET_SIZE_MIN 136

XXH128_hash_t XXH3_128bits(void const *const input, size_t const length);
XXH128_hash_t XXH3_128bits_withSeed(void const *const input, size_t const length, XXH64_hash_t const seed);
XXH128_hash_t XXH3_128bits_withSecret(void const *const input, size_t const length, void const *const secret, size_t secret_size);
XXH128_hash_t XXH128(void const *const input, size_t const length, XXH64_hash_t const seed);

#ifdef __cplusplus
}
#endif
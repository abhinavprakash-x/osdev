/*
 * Custom Memory Manipulation Library
 * Provides fundamental memory operations for the kernel environment where 
 * the standard C library is unavailable. 
 */

#include "mem.h"

void *memset(void *dest, int val, size_t count)
{
    size_t num_dwords = count / 4;
    size_t remainder = count % 4;

    uint32_t *d = (uint32_t *)dest;
    uint32_t v = (uint8_t)val;

    uint32_t fill = (v << 24) | (v << 16) | (v << 8) | v;
    for(size_t i = 0; i < num_dwords; ++i) d[i] = fill;

    uint8_t *d8 = (uint8_t *)(d + num_dwords);
    for(size_t i = 0; i < remainder; ++i) d8[i] = (uint8_t)val;

    return dest;
}

void *memcpy(void *dest, const void *src, size_t count)
{
    size_t num_dwords = count / 4;
    size_t remainder = count % 4;

    uint32_t *d = (uint32_t *)dest;
    const uint32_t *s = (uint32_t *)src;
    for(size_t i = 0; i < num_dwords; ++i) d[i] = s[i];

    uint8_t *d8 = (uint8_t *)(d + num_dwords);
    const uint8_t *s8 = (const uint8_t *)(s + num_dwords);
    for(size_t i = 0; i < remainder; ++i) d8[i] = s8[i];

    return dest;
}
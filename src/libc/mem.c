/*
 * Custom Memory Manipulation Library
 * Provides fundamental memory operations for the kernel environment where 
 * the standard C library is unavailable. 
 */

#include "mem.h"

void *memset(void *dest, int val, size_t count)
{
    uint8_t *d = (uint8_t *)dest;
    for(size_t i = 0; i < count; ++i) d[i] = (uint8_t)val;
    return dest;
}

void *memcpy(void *dest, const void *src, size_t count)
{
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (uint8_t *)src;
    for(size_t i = 0; i < count; ++i) d[i] = s[i];
    return dest;
}
#pragma once
#include <stdlib.h>
#include <stdint.h>

static inline int32_t syscall(uint32_t num, uint32_t arg1, uint32_t arg2)
{
    int32_t ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "b"(arg1), "c"(arg2)
        : "memory"
    );
    return ret;
}
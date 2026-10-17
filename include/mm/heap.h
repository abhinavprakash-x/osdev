#pragma once

#include <stdint.h>
#include <stddef.h>

#define HEAP_START    0x10000000u
#define HEAP_MAX_ADDR 0x20000000u
#define HEAP_MAX_SIZE (HEAP_MAX_ADDR - HEAP_START)
#define HEAP_MAGIC    0xDEADBEEFu

typedef struct heap_block {
    uint32_t magic;
    size_t size;
    int is_free;
    struct heap_block* next;
} heap_block_t;

void heap_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);
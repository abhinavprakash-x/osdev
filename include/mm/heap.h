#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define HEAP_START    0xC0000000u
#define HEAP_MAX_ADDR 0xE0000000u
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
bool heap_is_payload_range(uint32_t addr, size_t size);
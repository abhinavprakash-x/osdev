#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct heap_block {
    uint32_t magic;
    size_t size;
    int is_free;
    struct heap_block* next;
} heap_block_t;

void heap_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);
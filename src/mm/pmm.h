#pragma once
#include <stdint.h>

struct mem_map_entry{
    uint64_t base_addr;
    uint64_t region_length;
    uint32_t region_type;
    uint32_t acpi_attributes;
} __attribute__((packed));

void set_bit(int bit);
void clear_bit(int bit);
int test_bit(int bit);
void pmm_init();
void* pmm_alloc_block();
void pmm_free_block(void* physical_addr);
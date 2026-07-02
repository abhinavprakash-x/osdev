#pragma once
#include <stdint.h>
#include "pmm.h"

#define PTE_PRESENT 0x01
#define PTE_RW 0x02
#define PTE_USER 0x04

extern void load_page_directory(uint32_t* dir);
extern void enable_paging();

void paging_init();
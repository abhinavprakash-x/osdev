#pragma once
#include <stdint.h>

// Page Table / Page Directory Flags
#define PTE_PRESENT 0x01 // Page is present in memory
#define PTE_RW      0x02 // Page is read/write (if 0, read-only)
#define PTE_USER    0x04 // Page is accessible in User Mode (Ring 3)

// Architecture Constants
#define PAGE_SIZE   4096 // 4KB pages
#define PT_ENTRIES  1024 // 1024 entries per table/directory

extern void load_page_directory(uint32_t* dir);
extern void enable_paging(void);

void paging_init(void);
void map_page(uint32_t virtual_addr, uint32_t physical_addr);
void unmap_page(uint32_t virtual_addr);
uint32_t get_physical_addr(uint32_t virtual_addr);
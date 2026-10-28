#pragma once
#include <stdint.h>
#include <stdbool.h>

// Page Table / Page Directory Flags
#define PTE_PRESENT 0x01u // Page is present in memory
#define PTE_RW      0x02u // Page is read/write (if 0, read-only)
#define PTE_USER    0x04u // Page is accessible in User Mode (Ring 3)
#define PTE_SHARED  0x200u // OS-available bit 9: frame is borrowed/shared (do not free on teardown)

// Architecture Constants
#define PAGE_SIZE         4096u // 4KB pages
#define PT_ENTRIES        1024u // 1024 entries per table/directory
#define USER_SPACE_START  0x00400000u
#define USER_SPACE_END    0xBFFFFFFFu
#define PAGING_TEMP_VADDR 0x003FF000u

extern void load_page_directory(uint32_t* dir);
extern void enable_paging(void);

void paging_init(void);
void* paging_temp_map(uint32_t physical_addr);
void paging_temp_unmap(void);

bool map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
void unmap_page(uint32_t virtual_addr);
uint32_t get_physical_addr(uint32_t virtual_addr);

uint32_t* paging_get_kernel_directory(void);
uint32_t* paging_create_address_space(void);
void paging_destroy_address_space(uint32_t* directory);

void paging_switch_directory(uint32_t* directory);
bool paging_map_page(uint32_t* directory, uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);

bool user_range_valid(uint32_t virtual_addr, uint32_t size);
bool paging_is_mapped_range(uint32_t virtual_addr, uint32_t size, bool require_writable);

bool paging_alloc_pages(uint32_t* directory, uint32_t start_vaddr, uint32_t num_pages, uint32_t flags);
void paging_free_pages(uint32_t* directory, uint32_t start_vaddr, uint32_t num_pages);
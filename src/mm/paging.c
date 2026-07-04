/*
 * Virtual Memory Manager (Paging)
 * Translates virtual addresses to physical RAM addresses.
 * Currently configured to identity-map the first 4MB of RAM.
 */

#include "paging.h"
#include "pmm.h"
#include "../libc/mem.h"

void paging_init(void)
{
    // Allocate a 4KB block for the Page Directory and zero all entries
    uint32_t* page_directory = pmm_alloc_block();
    memset(page_directory, 0, PT_ENTRIES * sizeof(uint32_t));

    // Allocate a 4KB block for our first Page Table
    uint32_t* page_table = pmm_alloc_block();

    // Identity map the first 4MB (0x0 to 0x3FFFFF)
    for(int i = 0; i < PT_ENTRIES; ++i)
    {
        uint32_t physical_addr = i * PAGE_SIZE;
        page_table[i] = physical_addr | PTE_PRESENT | PTE_RW;
    }
    
    // Insert the newly created Page Table into the first slot of the Page Directory
    page_directory[0] = (uint32_t)page_table | PTE_PRESENT | PTE_RW;

    load_page_directory(page_directory);
    enable_paging();
}
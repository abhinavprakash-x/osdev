#include "paging.h"

void paging_init()
{
    uint32_t* page_directory = pmm_alloc_block();
    for(int i = 0; i < 1024; ++i) page_directory[i] = 0;

    uint32_t* page_table = pmm_alloc_block();
    for(int i = 0; i < 1024; ++i)
    {
        int physical_addr = i * 4096;
        page_table[i] = physical_addr | PTE_PRESENT | PTE_RW;
    }
    
    page_directory[0] = (uint32_t)page_table | PTE_PRESENT | PTE_RW;

    load_page_directory(page_directory);
    enable_paging();
}
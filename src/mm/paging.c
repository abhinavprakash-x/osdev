/*
 * Virtual Memory Manager (Paging)
 * Translates virtual addresses to physical RAM addresses.
 * Currently configured to identity-map the first 4MB of RAM.
 */

#include "paging.h"
#include "pmm.h"
#include "../libc/mem.h"

static uint32_t* page_directory = 0;

void paging_init(void)
{
    // Allocate a 4KB block for the Page Directory and zero all entries
    page_directory = pmm_alloc_block();
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

    // Recursive Paging
    page_directory[1023] = (uint32_t)page_directory | PTE_PRESENT | PTE_RW;

    load_page_directory(page_directory);
    enable_paging();
}

void map_page(uint32_t virtual_addr, uint32_t physical_addr)
{
    uint32_t page_dir_idx = (virtual_addr >> 22) & 0x3FF;
    uint32_t page_table_idx = (virtual_addr >> 12) & 0x3FF;

    uint32_t entry = physical_addr | PTE_RW | PTE_PRESENT;
    uint32_t* v_page_dir = (uint32_t*)0xFFFFF000;

    if((v_page_dir[page_dir_idx] & PTE_PRESENT) == 0) {
        uint32_t physical_new_table = (uint32_t)pmm_alloc_block();
        v_page_dir[page_dir_idx] = physical_new_table | PTE_RW | PTE_PRESENT;

        // Use the magic virtual address to zero out the newly mapped table!
        uint32_t* v_new_table = (uint32_t*)(0xFFC00000 + (page_dir_idx * PAGE_SIZE));
        memset(v_new_table, 0, PAGE_SIZE);
    }

    uint32_t* pt = (uint32_t*)(0xFFC00000 + (page_dir_idx * PAGE_SIZE));
    pt[page_table_idx] = entry;

    __asm__ volatile ("invlpg (%0)" : : "b"(virtual_addr) : "memory");
}

void unmap_page(uint32_t virtual_addr)
{
    uint32_t page_dir_idx = (virtual_addr >> 22) & 0x3FF;
    uint32_t page_table_idx = (virtual_addr >> 12) & 0x3FF;

    // Access the Page Directory using the recursive pointer
    uint32_t* v_page_dir = (uint32_t*)0xFFFFF000;

    // If the directory entry isn't present, the page is already unmapped
    if((v_page_dir[page_dir_idx] & PTE_PRESENT) == 0) {
        return; 
    }
    
    uint32_t* pt = (uint32_t*)(0xFFC00000 + (page_dir_idx * PAGE_SIZE));
    pt[page_table_idx] = 0;

    __asm__ volatile ("invlpg (%0)" : : "b"(virtual_addr) : "memory");
}

uint32_t get_physical_addr(uint32_t virtual_addr)
{
    uint32_t page_dir_idx = (virtual_addr >> 22) & 0x3FF;
    uint32_t page_table_idx = (virtual_addr >> 12) & 0x3FF;
    uint32_t offset = virtual_addr & 0xFFF; 

    uint32_t* v_page_dir = (uint32_t*)0xFFFFF000;
    if((v_page_dir[page_dir_idx] & PTE_PRESENT) == 0) {
        return 0; // Page is not mapped
    }

    uint32_t* pt = (uint32_t*)(0xFFC00000 + (page_dir_idx * PAGE_SIZE));

    if((pt[page_table_idx] & PTE_PRESENT) == 0) {
        return 0; // Page is not mapped
    }


    uint32_t physical_frame = pt[page_table_idx] & 0xFFFFF000;
    return physical_frame + offset;
}
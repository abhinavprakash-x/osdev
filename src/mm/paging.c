/*
 * Virtual Memory Manager (Paging)
 * Translates virtual addresses to physical RAM addresses.
 * Currently configured to identity-map the first 4MB of RAM.
 */

#include "paging.h"
#include "pmm.h"
#include "../libc/mem.h"

static uint32_t* kernel_page_directory = 0;
static uint32_t* current_page_directory = 0;

void paging_init(void)
{
    // Allocate a 4KB block for the Page Directory and zero all entries
    kernel_page_directory = pmm_alloc_block();
    if (kernel_page_directory == 0)
    {
        while(1) __asm__ volatile ("cli; hlt");
    }

    memset(kernel_page_directory, 0, PT_ENTRIES * sizeof(uint32_t));

    // Allocate a 4KB block for our first Page Table
    uint32_t* page_table = pmm_alloc_block();
    if (page_table == 0)
    {
        while(1) __asm__ volatile ("cli; hlt");
    }

    // Identity map the first 4MB (0x0 to 0x3FFFFF)
    for(int i = 0; i < PT_ENTRIES; ++i)
    {
        uint32_t physical_addr = i * PAGE_SIZE;
        page_table[i] = physical_addr | PTE_PRESENT | PTE_RW;
    }
    
    // Insert the newly created Page Table into the first slot of the Page Directory
    kernel_page_directory[0] = (uint32_t)page_table | PTE_PRESENT | PTE_RW;

    // Recursive Paging
    kernel_page_directory[1023] = (uint32_t)kernel_page_directory | PTE_PRESENT | PTE_RW;
    current_page_directory = kernel_page_directory;

    load_page_directory(kernel_page_directory);
    enable_paging();
}

bool map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags)
{
    uint32_t page_dir_idx = (virtual_addr >> 22) & 0x3FF;
    uint32_t page_table_idx = (virtual_addr >> 12) & 0x3FF;

    uint32_t entry = physical_addr | flags;
    uint32_t* v_page_dir = (uint32_t*)0xFFFFF000;

    if((v_page_dir[page_dir_idx] & PTE_PRESENT) == 0)
    {
        uint32_t physical_new_table = (uint32_t)pmm_alloc_block();
        if(physical_new_table == 0)
        {
            return false; // Allocation failed
        }

        v_page_dir[page_dir_idx] = physical_new_table | PTE_RW | PTE_PRESENT | (flags & PTE_USER);

        // Use the magic virtual address to zero out the newly mapped table!
        uint32_t* v_new_table = (uint32_t*)(0xFFC00000 + (page_dir_idx * PAGE_SIZE));
        memset(v_new_table, 0, PAGE_SIZE);
    }
    else if((flags & PTE_USER) && !(v_page_dir[page_dir_idx] & PTE_USER))
    {
        return false; // Cannot map user page in a kernel-only table
    }

    uint32_t* pt = (uint32_t*)(0xFFC00000 + (page_dir_idx * PAGE_SIZE));
    pt[page_table_idx] = entry;

    __asm__ volatile ("invlpg (%0)" : : "b"(virtual_addr) : "memory");
    return true;
}

void unmap_page(uint32_t virtual_addr)
{
    uint32_t page_dir_idx = (virtual_addr >> 22) & 0x3FF;
    uint32_t page_table_idx = (virtual_addr >> 12) & 0x3FF;

    // Access the Page Directory using the recursive pointer
    uint32_t* v_page_dir = (uint32_t*)0xFFFFF000;

    // If the directory entry isn't present, the page is already unmapped
    if((v_page_dir[page_dir_idx] & PTE_PRESENT) == 0)
    {
        return; 
    }
    
    uint32_t* pt = (uint32_t*)(0xFFC00000 + (page_dir_idx * PAGE_SIZE));
    if((pt[page_table_idx] & PTE_PRESENT) == 0)
    {
        return; // Page is already unmapped
    }

    pt[page_table_idx] = 0;
    __asm__ volatile ("invlpg (%0)" : : "b"(virtual_addr) : "memory");

    if ((v_page_dir[page_dir_idx] & PTE_USER) == 0)
    {
        return;
    }

    for (uint32_t i = 0; i < PT_ENTRIES; ++i)
    {
        if ((pt[i] & PTE_PRESENT) != 0)
        {
            return; // Found a present page, so don't free the table
        }
    }

    uint32_t page_table_phys = v_page_dir[page_dir_idx] & 0xFFFFF000;
    v_page_dir[page_dir_idx] = 0;
    pmm_free_block((void*)page_table_phys);
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

uint32_t* paging_get_kernel_directory(void)
{
    return kernel_page_directory;
}

uint32_t* paging_create_address_space(void)
{
    uint32_t* new_directory = pmm_alloc_block();
    if (new_directory == 0) return 0;

    memset(new_directory, 0, PAGE_SIZE);

    /*
     * Copy kernel mappings only.
     * Any PDE marked USER belongs to the current process
     * and must NOT be copied.
    */
    for (uint32_t i = 0; i < 1023; ++i)
    {
        uint32_t entry = kernel_page_directory[i];
        if ((entry & PTE_PRESENT) && !(entry & PTE_USER))
        {
            new_directory[i] = entry;
        }
    }

    /*
     * Recursive mapping for this address space.
     */
    new_directory[1023] = (uint32_t)new_directory | PTE_PRESENT | PTE_RW;
    return new_directory;
}

void paging_switch_directory(uint32_t* directory)
{
    current_page_directory = directory;
    load_page_directory(directory);
}

bool paging_map_page(uint32_t* directory, uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags)
{
    uint32_t* old_directory = current_page_directory;
    if (old_directory != directory) paging_switch_directory(directory);

    bool result = map_page(virtual_addr, physical_addr, flags);
    if (old_directory != directory) paging_switch_directory(old_directory);
    return result;
}

bool user_range_valid(uint32_t virtual_addr, uint32_t size)
{
    if (size == 0) return true;

    uint32_t end = virtual_addr + size - 1;

    // Detect 32-bit address wraparound.
    if (end < virtual_addr) return false;
    if (virtual_addr > USER_SPACE_END || end > USER_SPACE_END) return false;

    uint32_t first_page = virtual_addr & 0xFFFFF000;
    uint32_t last_page  = end & 0xFFFFF000;
    uint32_t* v_page_dir = (uint32_t*)0xFFFFF000;

    for (uint32_t page = first_page;; page += PAGE_SIZE)
    {
        uint32_t page_dir_idx  = (page >> 22) & 0x3FF;
        uint32_t page_table_idx = (page >> 12) & 0x3FF;

        uint32_t pde = v_page_dir[page_dir_idx];

        if ((pde & PTE_PRESENT) == 0 || (pde & PTE_USER) == 0)
        {
            return false;
        }

        uint32_t* pt = (uint32_t*)(0xFFC00000 + page_dir_idx * PAGE_SIZE);
        uint32_t pte = pt[page_table_idx];

        if ((pte & PTE_PRESENT) == 0 || (pte & PTE_USER) == 0)
        {
            return false;
        }

        if (page == last_page) break;
    }

    return true;
}

void paging_destroy_address_space(uint32_t* directory)
{
    if (directory == 0) return;
    if (directory == kernel_page_directory) return;

    if (current_page_directory == directory)
    {
        paging_switch_directory(kernel_page_directory);
    }

    paging_switch_directory(directory);
    uint32_t* v_page_dir = (uint32_t*)0xFFFFF000;

    for (uint32_t i = 0; i < 1023; ++i)
    {
        uint32_t pde = v_page_dir[i];

        if ((pde & PTE_PRESENT) == 0) continue;
        if ((pde & PTE_USER) == 0) continue;

        uint32_t page_table_phys = pde & 0xFFFFF000;
        uint32_t* page_table =(uint32_t*)(0xFFC00000 + (i * PAGE_SIZE));

        for (uint32_t j = 0; j < PT_ENTRIES; ++j)
        {
            uint32_t pte = page_table[j];
            if ((pte & PTE_PRESENT) == 0) continue;

            uint32_t physical_page = pte & 0xFFFFF000;
            if (physical_page != 0) pmm_free_block((void*)physical_page);

            page_table[j] = 0;
        }

        pmm_free_block((void*)page_table_phys);
        v_page_dir[i] = 0;
    }

    paging_switch_directory(kernel_page_directory);
    pmm_free_block((void*)directory);
}
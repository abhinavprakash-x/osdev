/*
 * Virtual Memory Manager (Paging)
 * Translates virtual addresses to physical RAM addresses.
 */

#include "mm/paging.h"
#include "mm/pmm.h"
#include "mm/heap.h"
#include "libc/mem.h"

static uint32_t* kernel_page_directory = 0;
static uint32_t* current_page_directory = 0;

void* paging_temp_map(uint32_t physical_addr)
{
    uint32_t page_dir_idx = (PAGING_TEMP_VADDR >> 22) & 0x3FFu;
    uint32_t page_table_idx = (PAGING_TEMP_VADDR >> 12) & 0x3FFu;
    uint32_t* pt = (uint32_t*)(0xFFC00000u + (page_dir_idx * PAGE_SIZE));

    pt[page_table_idx] = (physical_addr & 0xFFFFF000u) | PTE_PRESENT | PTE_RW;
    __asm__ volatile ("invlpg (%0)" : : "b"(PAGING_TEMP_VADDR) : "memory");
    return (void*)PAGING_TEMP_VADDR;
}

void paging_temp_unmap(void)
{
    uint32_t page_dir_idx = (PAGING_TEMP_VADDR >> 22) & 0x3FFu;
    uint32_t page_table_idx = (PAGING_TEMP_VADDR >> 12) & 0x3FFu;
    uint32_t* pt = (uint32_t*)(0xFFC00000u + (page_dir_idx * PAGE_SIZE));

    pt[page_table_idx] = 0;
    __asm__ volatile ("invlpg (%0)" : : "b"(PAGING_TEMP_VADDR) : "memory");
}

void paging_init(void)
{
    kernel_page_directory = pmm_alloc_block();
    if (kernel_page_directory == 0)
    {
        while(1) __asm__ volatile ("cli; hlt");
    }

    memset(kernel_page_directory, 0, PT_ENTRIES * sizeof(uint32_t));

    uint32_t* page_table = pmm_alloc_block();
    if (page_table == 0)
    {
        while(1) __asm__ volatile ("cli; hlt");
    }

    // Identity map the first 4MB except PAGING_TEMP_VADDR (PTE 1023 at 0x003FF000)
    for (uint32_t i = 0; i < PT_ENTRIES - 1; ++i)
    {
        uint32_t physical_addr = i * PAGE_SIZE;
        page_table[i] = physical_addr | PTE_PRESENT | PTE_RW;
    }
    page_table[PT_ENTRIES - 1] = 0;

    kernel_page_directory[0] = (uint32_t)page_table | PTE_PRESENT | PTE_RW;
    kernel_page_directory[1023] = (uint32_t)kernel_page_directory | PTE_PRESENT | PTE_RW;
    current_page_directory = kernel_page_directory;

    load_page_directory(kernel_page_directory);
    enable_paging();
}

bool map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags)
{
    // BUG-022: Enforce 4KB page alignment on both virtual and physical addresses
    if ((virtual_addr & 0xFFFu) != 0 || (physical_addr & 0xFFFu) != 0)
        return false;

    // Reject mapping over the recursive region or reserved temporary slot
    if (virtual_addr >= 0xFFC00000u || virtual_addr == PAGING_TEMP_VADDR)
        return false;

    // User-accessible pages must reside within [USER_SPACE_START, USER_SPACE_END]
    // and may never overlap the kernel heap [HEAP_START, HEAP_MAX_ADDR).
    if ((flags & PTE_USER) != 0)
    {
        if(virtual_addr < USER_SPACE_START || virtual_addr > USER_SPACE_END) return false;
        if(virtual_addr >= HEAP_START && virtual_addr < HEAP_MAX_ADDR) return false;
    }

    // BUG-023: Validate that the physical frame is usable and currently allocated in PMM
    if (!pmm_is_allocated_block((void*)physical_addr))
        return false;

    uint32_t page_dir_idx = (virtual_addr >> 22) & 0x3FFu;
    uint32_t page_table_idx = (virtual_addr >> 12) & 0x3FFu;

    uint32_t allowed_flags = flags & (PTE_PRESENT | PTE_RW | PTE_USER | PTE_SHARED);
    uint32_t entry = physical_addr | allowed_flags;
    uint32_t* v_page_dir = (uint32_t*)0xFFFFF000u;

    if ((v_page_dir[page_dir_idx] & PTE_PRESENT) == 0)
    {
        uint32_t physical_new_table = (uint32_t)pmm_alloc_block();
        if (physical_new_table == 0)
        {
            return false;
        }

        uint32_t pde = physical_new_table | PTE_RW | PTE_PRESENT | (allowed_flags & PTE_USER);
        v_page_dir[page_dir_idx] = pde;

        // BUG-016: Keep master kernel_page_directory updated with new kernel PDEs
        if ((allowed_flags & PTE_USER) == 0 && kernel_page_directory != 0)
        {
            kernel_page_directory[page_dir_idx] = pde;
        }

        uint32_t* v_new_table = (uint32_t*)(0xFFC00000u + (page_dir_idx * PAGE_SIZE));
        __asm__ volatile ("invlpg (%0)" : : "b"((uint32_t)v_new_table) : "memory");
        memset(v_new_table, 0, PAGE_SIZE);
    }
    else if ((allowed_flags & PTE_USER) && !(v_page_dir[page_dir_idx] & PTE_USER))
    {
        return false; // Cannot map user page in a kernel-only table
    }

    uint32_t* pt = (uint32_t*)(0xFFC00000u + (page_dir_idx * PAGE_SIZE));

    // BUG-020: Refuse to silently overwrite an already-present mapping
    if ((pt[page_table_idx] & PTE_PRESENT) != 0)
        return false;

    pt[page_table_idx] = entry;

    __asm__ volatile ("invlpg (%0)" : : "b"(virtual_addr) : "memory");
    return true;
}

void unmap_page(uint32_t virtual_addr)
{
    // BUG-022: Ignore unaligned or recursive addresses
    if ((virtual_addr & 0xFFFu) != 0 || virtual_addr >= 0xFFC00000u)
        return;

    uint32_t page_dir_idx = (virtual_addr >> 22) & 0x3FFu;
    uint32_t page_table_idx = (virtual_addr >> 12) & 0x3FFu;

    uint32_t* v_page_dir = (uint32_t*)0xFFFFF000u;

    if ((v_page_dir[page_dir_idx] & PTE_PRESENT) == 0)
        return;

    uint32_t* pt = (uint32_t*)(0xFFC00000u + (page_dir_idx * PAGE_SIZE));
    if ((pt[page_table_idx] & PTE_PRESENT) == 0)
        return;

    pt[page_table_idx] = 0;
    __asm__ volatile ("invlpg (%0)" : : "b"(virtual_addr) : "memory");

    // Never reclaim PDE 0 (the first 4 MiB identity-mapped kernel table)
    if(page_dir_idx == 0) return;

    for (uint32_t i = 0; i < PT_ENTRIES; ++i)
    {
        if ((pt[i] & PTE_PRESENT) != 0)
            return;
    }

    uint32_t pde = v_page_dir[page_dir_idx];
    uint32_t page_table_phys = pde & 0xFFFFF000u;
    v_page_dir[page_dir_idx] = 0;

    if ((pde & PTE_USER) == 0 && kernel_page_directory != 0)
    {
        kernel_page_directory[page_dir_idx] = 0;
    }

    // BUG-021: Invalidate the recursive page-table alias after reclaiming the table
    __asm__ volatile ("invlpg (%0)" : : "b"((uint32_t)pt) : "memory");
    pmm_free_block((void*)page_table_phys);
}

uint32_t get_physical_addr(uint32_t virtual_addr)
{
    uint32_t page_dir_idx = (virtual_addr >> 22) & 0x3FFu;
    uint32_t page_table_idx = (virtual_addr >> 12) & 0x3FFu;
    uint32_t offset = virtual_addr & 0xFFFu;

    uint32_t* v_page_dir = (uint32_t*)0xFFFFF000u;
    if ((v_page_dir[page_dir_idx] & PTE_PRESENT) == 0)
        return 0;

    uint32_t* pt = (uint32_t*)(0xFFC00000u + (page_dir_idx * PAGE_SIZE));
    if ((pt[page_table_idx] & PTE_PRESENT) == 0)
        return 0;

    uint32_t physical_frame = pt[page_table_idx] & 0xFFFFF000u;
    return physical_frame + offset;
}

uint32_t* paging_get_kernel_directory(void)
{
    return kernel_page_directory;
}

uint32_t* paging_create_address_space(void)
{
    uint32_t* new_directory_phys = pmm_alloc_block();
    if (new_directory_phys == 0) return 0;

    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    // BUG-043: Initialize new page directory via temporary virtual mapping
    // so physical frames >= 4 MiB never cause a page fault.
    uint32_t* new_directory = (uint32_t*)paging_temp_map((uint32_t)new_directory_phys);
    memset(new_directory, 0, PAGE_SIZE);

    for (uint32_t i = 0; i < 1023; ++i)
    {
        uint32_t entry = kernel_page_directory[i];
        if ((entry & PTE_PRESENT) && !(entry & PTE_USER))
        {
            new_directory[i] = entry;
        }
    }

    new_directory[1023] = (uint32_t)new_directory_phys | PTE_PRESENT | PTE_RW;
    paging_temp_unmap();

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
    return new_directory_phys;
}

void paging_switch_directory(uint32_t* directory)
{
    if (directory == 0) return;

    // BUG-016: Synchronize kernel PDEs into the target directory BEFORE loading CR3
    if (directory != kernel_page_directory && kernel_page_directory != 0)
    {
        if (directory == current_page_directory)
        {
            uint32_t* v_page_dir = (uint32_t*)0xFFFFF000u;
            for (uint32_t i = 0; i < 1023; ++i)
            {
                if ((v_page_dir[i] & PTE_USER) == 0)
                {
                    v_page_dir[i] = kernel_page_directory[i];
                }
            }
        }
        else
        {
            uint32_t eflags;
            __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

            uint32_t* target_dir = (uint32_t*)paging_temp_map((uint32_t)directory);
            for (uint32_t i = 0; i < 1023; ++i)
            {
                if ((target_dir[i] & PTE_USER) == 0)
                {
                    target_dir[i] = kernel_page_directory[i];
                }
            }
            paging_temp_unmap();

            __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
        }
    }

    current_page_directory = directory;
    load_page_directory(directory);
}

bool paging_map_page(uint32_t* directory, uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags)
{
    if (directory == 0) return false;

    // BUG-045: Disable interrupts while CR3 is temporarily switched so
    // a timer preemption cannot reload current_task->page_directory mid-map.
    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    uint32_t* old_directory = current_page_directory;
    if (old_directory != directory) paging_switch_directory(directory);

    bool result = map_page(virtual_addr, physical_addr, flags);
    if (old_directory != directory) paging_switch_directory(old_directory);

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
    return result;
}

bool user_range_valid(uint32_t virtual_addr, uint32_t size)
{
    if (size == 0) return true;

    uint32_t end = virtual_addr + size - 1;

    if (end < virtual_addr) return false;
    if (virtual_addr < USER_SPACE_START || virtual_addr > USER_SPACE_END || end > USER_SPACE_END)
        return false;
    if (virtual_addr < HEAP_MAX_ADDR && end >= HEAP_START)
        return false;

    uint32_t first_page = virtual_addr & 0xFFFFF000u;
    uint32_t last_page  = end & 0xFFFFF000u;
    uint32_t* v_page_dir = (uint32_t*)0xFFFFF000u;

    for (uint32_t page = first_page;; page += PAGE_SIZE)
    {
        uint32_t page_dir_idx   = (page >> 22) & 0x3FFu;
        uint32_t page_table_idx = (page >> 12) & 0x3FFu;

        uint32_t pde = v_page_dir[page_dir_idx];

        if ((pde & PTE_PRESENT) == 0 || (pde & PTE_USER) == 0)
            return false;

        uint32_t* pt = (uint32_t*)(0xFFC00000u + page_dir_idx * PAGE_SIZE);
        uint32_t pte = pt[page_table_idx];

        if ((pte & PTE_PRESENT) == 0 || (pte & PTE_USER) == 0)
            return false;

        if (page == last_page) break;
    }

    return true;
}

void paging_destroy_address_space(uint32_t* directory)
{
    if (directory == 0) return;
    if (directory == kernel_page_directory) return;

    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    uint32_t* restore_dir = (current_page_directory == directory || current_page_directory == 0)
                            ? kernel_page_directory
                            : current_page_directory;

    paging_switch_directory(directory);
    uint32_t* v_page_dir = (uint32_t*)0xFFFFF000u;

    for (uint32_t i = 0; i < 1023; ++i)
    {
        uint32_t pde = v_page_dir[i];

        if ((pde & PTE_PRESENT) == 0) continue;
        if ((pde & PTE_USER) == 0) continue;

        uint32_t page_table_phys = pde & 0xFFFFF000u;
        uint32_t* page_table = (uint32_t*)(0xFFC00000u + (i * PAGE_SIZE));

        for (uint32_t j = 0; j < PT_ENTRIES; ++j)
        {
            uint32_t pte = page_table[j];
            if ((pte & PTE_PRESENT) == 0) continue;

            uint32_t physical_page = pte & 0xFFFFF000u;

            // BUG-024: Only free frames owned by this mapping (skip PTE_SHARED frames)
            if (physical_page != 0 && (pte & PTE_SHARED) == 0)
            {
                pmm_free_block((void*)physical_page);
            }

            page_table[j] = 0;
        }

        v_page_dir[i] = 0;
        __asm__ volatile ("invlpg (%0)" : : "b"((uint32_t)page_table) : "memory");
        pmm_free_block((void*)page_table_phys);
    }

    paging_switch_directory(restore_dir);
    pmm_free_block((void*)directory);

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
}

bool paging_is_mapped_range(uint32_t virtual_addr, uint32_t size, bool require_writable)
{
    if (size == 0) return false;

    uint32_t end = virtual_addr + size - 1u;
    if (end < virtual_addr) return false;

    // Disallow recursive mapping region and reserved temporary slot
    if (virtual_addr >= 0xFFC00000u || end >= 0xFFC00000u)
        return false;

    uint32_t first_page = virtual_addr & 0xFFFFF000u;
    uint32_t last_page  = end & 0xFFFFF000u;
    uint32_t* v_page_dir = (uint32_t*)0xFFFFF000u;

    for (uint32_t page = first_page;; page += PAGE_SIZE)
    {
        if (page == PAGING_TEMP_VADDR)
            return false;

        uint32_t page_dir_idx   = (page >> 22) & 0x3FFu;
        uint32_t page_table_idx = (page >> 12) & 0x3FFu;

        uint32_t pde = v_page_dir[page_dir_idx];
        if ((pde & PTE_PRESENT) == 0)
            return false;
        if (require_writable && (pde & PTE_RW) == 0)
            return false;

        uint32_t* pt = (uint32_t*)(0xFFC00000u + page_dir_idx * PAGE_SIZE);
        uint32_t pte = pt[page_table_idx];
        if ((pte & PTE_PRESENT) == 0)
            return false;
        if (require_writable && (pte & PTE_RW) == 0)
            return false;

        if (page == last_page) break;
    }

    return true;
}

bool paging_alloc_pages(uint32_t* directory, uint32_t start_vaddr, uint32_t num_pages, uint32_t flags)
{
    if (directory == 0 || num_pages == 0) return false;
    if ((start_vaddr & 0xFFFu) != 0) return false;

    uint64_t total_bytes = (uint64_t)num_pages * PAGE_SIZE;
    if (total_bytes > 0xFFFFFFFFULL || (start_vaddr + (uint32_t)(total_bytes - 1u)) < start_vaddr)
        return false;

    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    uint32_t allocated = 0;
    for (uint32_t i = 0; i < num_pages; ++i)
    {
        uint32_t vaddr = start_vaddr + (i * PAGE_SIZE);
        uint32_t phys = (uint32_t)pmm_alloc_block();
        if (phys == 0)
            break;

        void* temp = paging_temp_map(phys);
        memset(temp, 0, PAGE_SIZE);
        paging_temp_unmap();

        if (!paging_map_page(directory, vaddr, phys, flags | PTE_PRESENT))
        {
            pmm_free_block((void*)phys);
            break;
        }
        allocated++;
    }

    if (allocated < num_pages)
    {
        if (allocated > 0)
        {
            paging_free_pages(directory, start_vaddr, allocated);
        }
        __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
        return false;
    }

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
    return true;
}

void paging_free_pages(uint32_t* directory, uint32_t start_vaddr, uint32_t num_pages)
{
    if (directory == 0 || num_pages == 0) return;
    if ((start_vaddr & 0xFFFu) != 0) return;

    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    uint32_t* old_directory = current_page_directory;
    if (old_directory != directory) paging_switch_directory(directory);

    for (uint32_t i = 0; i < num_pages; ++i)
    {
        uint32_t vaddr = start_vaddr + (i * PAGE_SIZE);
        if (vaddr >= 0xFFC00000u) break;

        uint32_t page_dir_idx   = (vaddr >> 22) & 0x3FFu;
        uint32_t page_table_idx = (vaddr >> 12) & 0x3FFu;
        uint32_t* v_page_dir    = (uint32_t*)0xFFFFF000u;

        if ((v_page_dir[page_dir_idx] & PTE_PRESENT) == 0)
            continue;

        uint32_t* pt = (uint32_t*)(0xFFC00000u + (page_dir_idx * PAGE_SIZE));
        uint32_t pte = pt[page_table_idx];
        if ((pte & PTE_PRESENT) == 0)
            continue;

        uint32_t phys = pte & 0xFFFFF000u;
        bool is_shared_flag = (pte & PTE_SHARED) != 0;

        unmap_page(vaddr);

        if (phys != 0 && !is_shared_flag)
        {
            pmm_free_block((void*)phys);
        }
    }

    if (old_directory != directory) paging_switch_directory(old_directory);
    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
}
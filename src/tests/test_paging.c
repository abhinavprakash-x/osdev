/*
 * Virtual Memory / Paging Tests
 * Covers page mapping, address space cleanup, shared kernel PDE isolation,
 * and safe page-table reclamation (BUG-039).
 */

#include "tests/test_framework.h"
#include "mm/paging.h"
#include "mm/pmm.h"
#include "libc/stdlib.h"

static void test_paging_basic(void)
{
    printf("[Paging]\n");
    uint32_t phys = (uint32_t)pmm_alloc_block();

    test_assert_true(phys != 0, "paging(test frame allocation)");
    if (phys == 0) return;

    test_assert_true(map_page(0x20000000, phys, PTE_PRESENT | PTE_RW), "paging(map)");
    test_assert_equal_int((int)phys, (int)get_physical_addr(0x20000000), "paging(translate base)");
    test_assert_equal_int((int)(phys + 100), (int)get_physical_addr(0x20000064), "paging(translate offset)");

    unmap_page(0x20000000);
    test_assert_equal_int(0, (int)get_physical_addr(0x20000000), "paging(unmap)");

    pmm_free_block((void*)phys);
    printf("Paging tests complete.\n");
}

static void test_address_space_cleanup(void)
{
    printf("[Address Space Cleanup]\n");

    int before = get_used_memory();
    uint32_t* directory = paging_create_address_space();

    test_assert_true(directory != 0, "address space(create)");
    if (directory == 0) return;

    uint32_t phys = (uint32_t)pmm_alloc_block();
    test_assert_true(phys != 0, "address space(test frame)");
    if (phys == 0)
    {
        paging_destroy_address_space(directory);
        return;
    }

    bool mapped = paging_map_page(directory, 0x400000, phys, PTE_PRESENT | PTE_RW | PTE_USER);
    test_assert_true(mapped, "address space(map)");
    if (!mapped)
    {
        pmm_free_block((void*)phys);
        paging_destroy_address_space(directory);
        return;
    }

    paging_destroy_address_space(directory);
    int after = get_used_memory();

    test_assert_equal_int(before, after, "address space(cleanup)");
    printf("Address space cleanup tests complete.\n");
}

static void test_user_mapping_shared_kernel_pde(void)
{
    printf("[User Mapping Isolation]\n");

    uint32_t* directory = paging_create_address_space();
    test_assert_true(directory != 0, "user mapping(shared pde create)");
    if (directory == 0) return;

    uint32_t phys = (uint32_t)pmm_alloc_block();
    test_assert_true(phys != 0, "user mapping(shared pde frame)");
    if (phys == 0)
    {
        paging_destroy_address_space(directory);
        return;
    }

    bool mapped = paging_map_page(directory, 0x00100000, phys, PTE_PRESENT | PTE_RW | PTE_USER);
    test_assert_true(!mapped, "user mapping(shared pde rejected)");

    uint32_t kernel_pde = paging_get_kernel_directory()[0];
    test_assert_true((kernel_pde & PTE_USER) == 0, "user mapping(kernel pde unchanged)");

    pmm_free_block((void*)phys);
    paging_destroy_address_space(directory);

    printf("User mapping isolation tests complete.\n");
}

static void test_page_table_reclamation(void)
{
    printf("[Page Table Reclamation]\n");

    const uint32_t virtual_addr_1 = 0x00400000;
    const uint32_t virtual_addr_2 = 0x00401000;

    const uint32_t page_dir_idx     = (virtual_addr_1 >> 22) & 0x3FF;
    const uint32_t page_table_idx_1 = (virtual_addr_1 >> 12) & 0x3FF;
    const uint32_t page_table_idx_2 = (virtual_addr_2 >> 12) & 0x3FF;

    test_assert_true(page_dir_idx == ((virtual_addr_2 >> 22) & 0x3FF), "page table reclamation(same PDE)");
    test_assert_true(page_table_idx_1 != page_table_idx_2, "page table reclamation(different PTEs)");

    int used_before = get_used_memory();
    uint32_t* directory = paging_create_address_space();

    test_assert_true(directory != 0, "page table reclamation(create address space)");
    if (directory == 0) return;

    uint32_t phys1 = (uint32_t)pmm_alloc_block();
    uint32_t phys2 = (uint32_t)pmm_alloc_block();

    test_assert_true(phys1 != 0, "page table reclamation(frame 1)");
    test_assert_true(phys2 != 0, "page table reclamation(frame 2)");

    if (phys1 == 0 || phys2 == 0)
    {
        if (phys1 != 0) pmm_free_block((void*)phys1);
        if (phys2 != 0) pmm_free_block((void*)phys2);
        paging_destroy_address_space(directory);
        return;
    }

    bool mapped1 = paging_map_page(directory, virtual_addr_1, phys1, PTE_PRESENT | PTE_RW | PTE_USER);
    bool mapped2 = paging_map_page(directory, virtual_addr_2, phys2, PTE_PRESENT | PTE_RW | PTE_USER);

    test_assert_true(mapped1, "page table reclamation(map 1)");
    test_assert_true(mapped2, "page table reclamation(map 2)");

    if (!mapped1 || !mapped2)
    {
        pmm_free_block((void*)phys1);
        pmm_free_block((void*)phys2);
        paging_destroy_address_space(directory);
        return;
    }

    // BUG-045: Disable interrupts while CR3 is switched to `directory` so
    // timer preemption cannot reload current_task->page_directory mid-inspection.
    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    paging_switch_directory(directory);

    uint32_t* page_directory = (uint32_t*)0xFFFFF000;
    uint32_t* page_table = (uint32_t*)(0xFFC00000 + (page_dir_idx * PAGE_SIZE));

    /* STATE 1: Both mappings exist */
    bool s1_pde_present  = (page_directory[page_dir_idx] & PTE_PRESENT) != 0;
    bool s1_pde_user     = (page_directory[page_dir_idx] & PTE_USER) != 0;
    bool s1_pte1_present = (page_table[page_table_idx_1] & PTE_PRESENT) != 0;
    bool s1_pte2_present = (page_table[page_table_idx_2] & PTE_PRESENT) != 0;
    bool s1_pte1_phys    = (page_table[page_table_idx_1] & 0xFFFFF000) == phys1;
    bool s1_pte2_phys    = (page_table[page_table_idx_2] & 0xFFFFF000) == phys2;
    uint32_t page_table_phys = page_directory[page_dir_idx] & 0xFFFFF000;

    /* STATE 2: Remove first mapping; table survives because PTE #2 is still present */
    unmap_page(virtual_addr_1);

    bool s2_pte1_removed   = (page_table[page_table_idx_1] & PTE_PRESENT) == 0;
    bool s2_pte2_survives  = (page_table[page_table_idx_2] & PTE_PRESENT) != 0;
    bool s2_table_survives = (page_directory[page_dir_idx] & PTE_PRESENT) != 0;

    /* STATE 3 (BUG-039): Remove final mapping; table is reclaimed and PDE cleared */
    int used_before_table_reclaim = get_used_memory();
    unmap_page(virtual_addr_2);

    bool s3_pde_cleared       = (page_directory[page_dir_idx] == 0);
    uint32_t s3_vaddr2_phys   = get_physical_addr(virtual_addr_2);
    int used_after_reclaim    = get_used_memory();

    paging_switch_directory(paging_get_kernel_directory());
    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");

    test_assert_true(s1_pde_present, "page table reclamation(PDE present after mappings)");
    test_assert_true(s1_pde_user, "page table reclamation(PDE user)");
    test_assert_true(s1_pte1_present, "page table reclamation(PTE 1 present)");
    test_assert_true(s1_pte2_present, "page table reclamation(PTE 2 present)");
    test_assert_true(s1_pte1_phys, "page table reclamation(PTE 1 physical frame)");
    test_assert_true(s1_pte2_phys, "page table reclamation(PTE 2 physical frame)");
    test_assert_true(page_table_phys != 0, "page table reclamation(page table physical address)");

    test_assert_true(s2_pte1_removed, "page table reclamation(PTE 1 removed)");
    test_assert_true(s2_pte2_survives, "page table reclamation(PTE 2 survives)");
    test_assert_true(s2_table_survives, "page table reclamation(table survives non-empty)");

    test_assert_true(s3_pde_cleared, "page table reclamation(empty table PDE cleared)");
    test_assert_equal_int(0, (int)s3_vaddr2_phys, "page table reclamation(virtual_addr_2 unmapped)");
    test_assert_equal_int(used_before_table_reclaim - 1, used_after_reclaim, "page table reclamation(table frame freed)");

    pmm_free_block((void*)phys1);
    pmm_free_block((void*)phys2);
    paging_destroy_address_space(directory);

    int used_after = get_used_memory();
    test_assert_equal_int(used_before, used_after, "page table reclamation(PMM usage restored)");

    printf("Page table reclamation tests complete.\n");
}

static void test_paging_validation_and_ownership(void)
{
    printf("[Paging Validation & Ownership]\n");

    int used_before = get_used_memory();
    uint32_t phys1 = (uint32_t)pmm_alloc_block();
    uint32_t phys2 = (uint32_t)pmm_alloc_block();

    test_assert_true(phys1 != 0 && phys2 != 0, "paging validation(alloc test frames)");
    if (phys1 == 0 || phys2 == 0)
    {
        if (phys1) pmm_free_block((void*)phys1);
        if (phys2) pmm_free_block((void*)phys2);
        return;
    }

    // BUG-022: Unaligned virtual or physical addresses must be rejected
    test_assert_true(!map_page(0x20000001u, phys1, PTE_PRESENT | PTE_RW), "paging BUG-022(unaligned vaddr rejected)");
    test_assert_true(!map_page(0x20000000u, phys1 + 1, PTE_PRESENT | PTE_RW), "paging BUG-022(unaligned paddr rejected)");

    // BUG-023: Reserved (<1MB) or unallocated physical frames must be rejected
    test_assert_true(!map_page(0x20000000u, 0x00001000u, PTE_PRESENT | PTE_RW), "paging BUG-023(reserved frame rejected)");
    pmm_free_block((void*)phys2);
    test_assert_true(!map_page(0x20000000u, phys2, PTE_PRESENT | PTE_RW), "paging BUG-023(unallocated frame rejected)");

    // Re-allocate phys2 for overwrite test
    phys2 = (uint32_t)pmm_alloc_block();
    test_assert_true(phys2 != 0, "paging validation(realloc phys2)");

    // BUG-020: Overwriting an existing present mapping must be rejected without leaking
    test_assert_true(map_page(0x20000000u, phys1, PTE_PRESENT | PTE_RW), "paging BUG-020(initial map)");
    test_assert_true(!map_page(0x20000000u, phys2, PTE_PRESENT | PTE_RW), "paging BUG-020(overwrite rejected)");
    test_assert_equal_int((int)phys1, (int)get_physical_addr(0x20000000u), "paging BUG-020(original mapping preserved)");
    unmap_page(0x20000000u);

    // BUG-016 & BUG-024: Kernel PDE sync and shared-frame preservation on teardown
    uint32_t* dir = paging_create_address_space();
    test_assert_true(dir != 0, "paging BUG-016/024(create dir)");
    if (dir != 0)
    {
        bool mapped_kernel_pde = map_page(0x30000000u, phys1, PTE_PRESENT | PTE_RW);

        uint32_t eflags;
        __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");
        paging_switch_directory(dir);
        uint32_t translated_in_user_dir = get_physical_addr(0x30000000u);
        paging_switch_directory(paging_get_kernel_directory());
        __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");

        unmap_page(0x30000000u);

        test_assert_true(mapped_kernel_pde, "paging BUG-016(map new kernel PDE)");
        test_assert_equal_int((int)phys1, (int)translated_in_user_dir, "paging BUG-016(new kernel PDE synced to user dir)");

        // Map phys2 as PTE_SHARED in user address space; teardown must NOT free phys2
        test_assert_true(paging_map_page(dir, 0x00400000u, phys2, PTE_PRESENT | PTE_RW | PTE_USER | PTE_SHARED),
                         "paging BUG-024(map shared frame)");
        paging_destroy_address_space(dir);
        test_assert_true(pmm_is_allocated_block((void*)phys2), "paging BUG-024(shared frame survives teardown)");
    }

    pmm_free_block((void*)phys1);
    pmm_free_block((void*)phys2);
    test_assert_equal_int(used_before, get_used_memory(), "paging validation(PMM accounting restored)");

    uint32_t cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    test_assert_true((cr0 & 0x00010000u) != 0, "paging (CR0.WP write-protect enabled)");
}

void test_paging_suite(void)
{
    test_paging_basic();
    test_address_space_cleanup();
    test_user_mapping_shared_kernel_pde();
    test_page_table_reclamation();
    test_paging_validation_and_ownership();
}
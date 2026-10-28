/*
 * Physical Memory Manager (PMM) Tests
 * Covers allocation, free validation, and E820 boundary/rounding regressions
 * (BUG-007, BUG-008, BUG-012, BUG-025).
 */

#include "tests/test_framework.h"
#include "mm/pmm.h"
#include "libc/mem.h"
#include "libc/stdlib.h"

#define E820_COUNT_PTR   ((uint32_t*)0x5000)
#define E820_ENTRIES_PTR ((struct mem_map_entry*)0x5004)
#define MAX_SAVED_RANGES 512
#define MAX_SAVED_E820 32

static pmm_alloc_range_t saved_ranges[MAX_SAVED_RANGES];
static struct mem_map_entry orig_entries[MAX_SAVED_E820];

static void test_pmm_basic(void)
{
    printf("[Physical Memory Manager]\n");

    void *a = pmm_alloc_block();
    void *b = pmm_alloc_block();

    test_assert_true(a != 0, "pmm(first)");
    test_assert_true(b != 0, "pmm(second)");
    test_assert_true(a != b, "pmm(unique)");
    test_assert_true(((uint32_t)a % 4096) == 0, "pmm(aligned)");

    pmm_free_block(a);
    pmm_free_block(b);

    printf("PMM basic tests complete.\n");
}

static void test_pmm_free_validation(void)
{
    printf("[PMM Free Validation]\n");

    int used_before = get_used_memory();

    // Test 1: Normal allocation/free
    uint32_t phys = (uint32_t)pmm_alloc_block();
    test_assert_true(phys != 0, "pmm free validation(allocate)");
    if (phys == 0) return;

    int used_after_alloc = get_used_memory();
    test_assert_equal_int(used_before + 1, used_after_alloc, "pmm free validation(allocation accounting)");
    pmm_free_block((void*)phys);
    test_assert_equal_int(used_before, get_used_memory(), "pmm free validation(normal free)");

    // Test 2: Double free
    phys = (uint32_t)pmm_alloc_block();
    test_assert_true(phys != 0, "pmm free validation(double free allocate)");
    if (phys == 0) return;

    pmm_free_block((void*)phys);
    int used_after_first_free = get_used_memory();
    pmm_free_block((void*)phys);
    test_assert_equal_int(used_after_first_free, get_used_memory(), "pmm free validation(double free)");

    // Test 3: Unaligned physical address
    phys = (uint32_t)pmm_alloc_block();
    test_assert_true(phys != 0, "pmm free validation(unaligned allocate)");
    if (phys == 0) return;

    int used_before_invalid_free = get_used_memory();
    pmm_free_block((void*)(phys + 1));
    test_assert_equal_int(used_before_invalid_free, get_used_memory(), "pmm free validation(unaligned free rejected)");
    pmm_free_block((void*)phys);
    test_assert_equal_int(used_before, get_used_memory(), "pmm free validation(unaligned original frame preserved)");

    // Test 4: Reserved physical frame (< 1MB)
    int used_before_reserved_free = get_used_memory();
    pmm_free_block((void*)0x1000);
    test_assert_equal_int(used_before_reserved_free, get_used_memory(), "pmm free validation(reserved frame rejected)");

    uint32_t test_phys = (uint32_t)pmm_alloc_block();
    test_assert_true(test_phys != 0x1000, "pmm free validation(reserved frame not allocatable)");
    if (test_phys != 0) pmm_free_block((void*)test_phys);

    // Test 5: Reserved physical frame (above 1MB)
    uint32_t reserved_phys = 0;
    for (uint32_t frame = 256; frame < 1048576; ++frame)
    {
        uint32_t phys_addr = frame * 4096;
        if (!pmm_is_usable_block((void*)phys_addr))
        {
            reserved_phys = phys_addr;
            break;
        }
    }

    if (reserved_phys != 0)
    {
        int used_before_reserved_above_1mb = get_used_memory();
        pmm_free_block((void*)reserved_phys);
        test_assert_equal_int(used_before_reserved_above_1mb, get_used_memory(), "pmm free validation(reserved frame above 1MB rejected)");
        test_assert_true(!pmm_is_usable_block((void*)reserved_phys), "pmm free validation(reserved frame remains unusable)");
    }

    // Test 6: Allocated frames must come from usable memory
    uint32_t usable_phys = (uint32_t)pmm_alloc_block();
    test_assert_true(usable_phys != 0, "pmm free validation(allocated frame exists)");
    if (usable_phys != 0)
    {
        test_assert_true(pmm_is_usable_block((void*)usable_phys), "pmm free validation(allocated frame is usable)");
        pmm_free_block((void*)usable_phys);
    }

    // Test 7: Out-of-range address
    int used_before_invalid_address = get_used_memory();
    pmm_free_block((void*)0xFFFFFFFF);
    test_assert_equal_int(used_before_invalid_address, get_used_memory(), "pmm free validation(out of range rejected)");

    // Final invariant check
    test_assert_equal_int(used_before, get_used_memory(), "pmm free validation(final accounting)");

    printf("PMM free validation tests complete.\n");
}

/*
 * Regression tests for BUG-007 (E820 4GB clipping / overflow) and
 * BUG-025 (E820 start-upward / end-downward page rounding).
 */
static void test_pmm_e820_bounds_and_rounding(void)
{
    printf("[PMM E820 Clipping & Rounding]\n");

    int total_before = get_total_memory();
    int used_before  = get_used_memory();

    uint32_t saved_range_count = pmm_save_allocated_ranges(saved_ranges, MAX_SAVED_RANGES);
    test_assert_true(saved_range_count <= MAX_SAVED_RANGES, "e820 test(snapshot allocated blocks)");
    if (saved_range_count > MAX_SAVED_RANGES) return;

    uint32_t orig_e820_count = *E820_COUNT_PTR;
    test_assert_true(orig_e820_count <= MAX_SAVED_E820, "e820 test(snapshot e820 table)");
    if (orig_e820_count > MAX_SAVED_E820) return;

    memcpy(orig_entries, E820_ENTRIES_PTR, orig_e820_count * sizeof(struct mem_map_entry));

    // Preserve EFLAGS and disable interrupts while synthetic PMM state is active
    // FIX: Use AT&T syntax `pushfl` and `popfl` for GCC inline assembly
    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    // --- Case 1 (BUG-007): Region entirely above 4 GiB must be ignored ---
    *E820_COUNT_PTR = 1;
    E820_ENTRIES_PTR[0].base_addr       = 0x100000000ULL;
    E820_ENTRIES_PTR[0].region_length   = 0x000100000ULL;
    E820_ENTRIES_PTR[0].region_type     = 1;
    E820_ENTRIES_PTR[0].acpi_attributes = 1;
    pmm_init();
    int case1_total = get_total_memory();

    // --- Case 2 (BUG-007): Region crossing 4 GiB boundary clipped to 2 frames ---
    *E820_COUNT_PTR = 1;
    E820_ENTRIES_PTR[0].base_addr       = 0x0FFFFE000ULL;
    E820_ENTRIES_PTR[0].region_length   = 0x000010000ULL; // Crosses 0x100000000
    E820_ENTRIES_PTR[0].region_type     = 1;
    E820_ENTRIES_PTR[0].acpi_attributes = 1;
    pmm_init();
    int case2_total = get_total_memory();
    bool case2_frame_a = pmm_is_usable_block((void*)0xFFFFE000u);
    bool case2_frame_b = pmm_is_usable_block((void*)0xFFFFF000u);

    // --- Case 3 (BUG-007): 64-bit addition overflow (base + length wraps) ---
    *E820_COUNT_PTR = 1;
    E820_ENTRIES_PTR[0].base_addr       = 0x0FFFE0000ULL;
    E820_ENTRIES_PTR[0].region_length   = 0xFFFFFFFFFFFFFFFFULL;
    E820_ENTRIES_PTR[0].region_type     = 1;
    E820_ENTRIES_PTR[0].acpi_attributes = 1;
    pmm_init();
    int case3_total = get_total_memory(); // 0x100000000 - 0xFFFE0000 = 0x20000 (32 frames)

    // --- Case 4 (BUG-025): Unaligned start & end rounded inward + sub-page ignored ---
    // Entry 0: 0x200500 .. 0x202B00 -> rounds inward to 0x201000 .. 0x202000 (1 frame)
    // Entry 1: 0x300100 .. 0x300F00 -> sub-page, 0 complete frames
    *E820_COUNT_PTR = 2;
    E820_ENTRIES_PTR[0].base_addr       = 0x00200500ULL;
    E820_ENTRIES_PTR[0].region_length   = 0x00002600ULL;
    E820_ENTRIES_PTR[0].region_type     = 1;
    E820_ENTRIES_PTR[0].acpi_attributes = 1;

    E820_ENTRIES_PTR[1].base_addr       = 0x00300100ULL;
    E820_ENTRIES_PTR[1].region_length   = 0x00000E00ULL;
    E820_ENTRIES_PTR[1].region_type     = 1;
    E820_ENTRIES_PTR[1].acpi_attributes = 1;
    pmm_init();
    int case4_total = get_total_memory();
    bool case4_head_excluded = !pmm_is_usable_block((void*)0x00200000u);
    bool case4_inner_usable  =  pmm_is_usable_block((void*)0x00201000u);
    bool case4_tail_excluded = !pmm_is_usable_block((void*)0x00202000u);
    bool case4_sub_excluded  = !pmm_is_usable_block((void*)0x00300000u);

    // --- Case 5 (BUG-026): Disabled ACPI entry (bit 0 == 0) must be ignored ---
    *E820_COUNT_PTR = 1;
    E820_ENTRIES_PTR[0].base_addr       = 0x00200000ULL;
    E820_ENTRIES_PTR[0].region_length   = 0x00004000ULL; // 4 frames
    E820_ENTRIES_PTR[0].region_type     = 1;
    E820_ENTRIES_PTR[0].acpi_attributes = 0;             // Disabled!
    pmm_init();
    int case5_total = get_total_memory();

    // --- Case 6 (BUG-042): Overlapping usable E820 entries (e.g. frames 0..5 and 3..8) ---
    // Entry 0: 0x200000 .. 0x205000 (5 frames: 0x200000..0x204000)
    // Entry 1: 0x203000 .. 0x208000 (5 frames: 0x203000..0x207000, overlaps 0x203000 & 0x204000)
    // Total unique usable frames must be 8 (not 10).
    *E820_COUNT_PTR = 2;
    E820_ENTRIES_PTR[0].base_addr       = 0x00200000ULL;
    E820_ENTRIES_PTR[0].region_length   = 0x00005000ULL;
    E820_ENTRIES_PTR[0].region_type     = 1;
    E820_ENTRIES_PTR[0].acpi_attributes = 1;

    E820_ENTRIES_PTR[1].base_addr       = 0x00203000ULL;
    E820_ENTRIES_PTR[1].region_length   = 0x00005000ULL;
    E820_ENTRIES_PTR[1].region_type     = 1;
    E820_ENTRIES_PTR[1].acpi_attributes = 1;
    pmm_init();
    int case6_total = get_total_memory();

    // Restore original E820 table and live PMM allocations before re-enabling interrupts
    *E820_COUNT_PTR = orig_e820_count;
    memcpy(E820_ENTRIES_PTR, orig_entries, orig_e820_count * sizeof(struct mem_map_entry));
    pmm_init();
    pmm_restore_allocated_ranges(saved_ranges, saved_range_count);

    // FIX: Use AT&T syntax `pushfl` and `popfl` for GCC inline assembly
    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");

    test_assert_equal_int(0, case5_total, "e820 BUG-026(disabled ACPI entry ignored)");
    test_assert_equal_int(8, case6_total, "e820 BUG-042(overlapping usable entries deduplicated)");

    test_assert_equal_int(0, case1_total, "e820 BUG-007(ignore region >= 4GB)");
    test_assert_equal_int(2, case2_total, "e820 BUG-007(clip region straddling 4GB)");
    test_assert_true(case2_frame_a && case2_frame_b, "e820 BUG-007(clipped boundary frames usable)");
    test_assert_equal_int(32, case3_total, "e820 BUG-007(64-bit length overflow clipped safely)");

    test_assert_equal_int(1, case4_total, "e820 BUG-025(inward rounding frame count)");
    test_assert_true(case4_head_excluded, "e820 BUG-025(partial leading frame excluded)");
    test_assert_true(case4_inner_usable, "e820 BUG-025(complete inner frame usable)");
    test_assert_true(case4_tail_excluded, "e820 BUG-025(partial trailing frame excluded)");
    test_assert_true(case4_sub_excluded, "e820 BUG-025(sub-page region excluded)");

    test_assert_equal_int(total_before, get_total_memory(), "e820 test(total memory restored)");
    test_assert_equal_int(used_before, get_used_memory(), "e820 test(used memory restored)");

    printf("PMM E820 tests complete.\n");
}

static void test_pmm_refcounting(void)
{
    printf("[PMM Frame Reference Counting]\n");

    int used_before = get_used_memory();
    void* frame = pmm_alloc_block();

    test_assert_true(frame != 0, "pmm refcount(alloc block)");
    if (frame == 0) return;

    test_assert_equal_int(1, pmm_get_refcount(frame), "pmm refcount(initial refcount is 1)");
    test_assert_equal_int(2, pmm_ref_block(frame), "pmm refcount(increment to 2)");
    test_assert_equal_int(3, pmm_ref_block(frame), "pmm refcount(increment to 3)");
    test_assert_equal_int(3, pmm_get_refcount(frame), "pmm refcount(query returns 3)");

    // First free drops refcount 3 -> 2; frame remains allocated
    pmm_free_block(frame);
    test_assert_equal_int(2, pmm_get_refcount(frame), "pmm refcount(free drops 3 -> 2)");
    test_assert_true(pmm_is_allocated_block(frame), "pmm refcount(still allocated at ref 2)");

    // Second free drops refcount 2 -> 1; frame remains allocated
    pmm_free_block(frame);
    test_assert_equal_int(1, pmm_get_refcount(frame), "pmm refcount(free drops 2 -> 1)");
    test_assert_true(pmm_is_allocated_block(frame), "pmm refcount(still allocated at ref 1)");

    // Third free drops refcount 1 -> 0; frame is released to PMM
    pmm_free_block(frame);
    test_assert_equal_int(0, pmm_get_refcount(frame), "pmm refcount(final free drops to 0)");
    test_assert_true(!pmm_is_allocated_block(frame), "pmm refcount(frame released at ref 0)");
    test_assert_equal_int(used_before, get_used_memory(), "pmm refcount(PMM accounting restored)");

    printf("PMM refcount tests complete.\n");
}

void test_pmm_suite(void)
{
    test_pmm_basic();
    test_pmm_free_validation();
    test_pmm_e820_bounds_and_rounding();
    test_pmm_refcounting();
}
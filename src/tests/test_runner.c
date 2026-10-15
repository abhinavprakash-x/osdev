/*
 * Kernel Test Suite Runner
 * Coordinates test execution, tracks pass/fail counts, and prints summaries.
 */

#include "apps/kernel_tests.h"
#include "tests/test_framework.h"
#include "libc/stdlib.h"
#include "drivers/pit.h"

static int tests_passed = 0;
static int tests_failed = 0;

void test_reset_counters(void)
{
    tests_passed = 0;
    tests_failed = 0;
}

int test_get_passed(void)
{
    return tests_passed;
}

int test_get_failed(void)
{
    return tests_failed;
}

void test_assert_true(int cond, const char *name)
{
    if (cond)
    {
        tests_passed++;
        printf("[PASS] %s\n", name);
    }
    else
    {
        tests_failed++;
        printf("[FAIL] %s\n", name);
    }
}

void test_assert_equal_int(int expected, int actual, const char *name)
{
    if (expected == actual)
    {
        tests_passed++;
        printf("[PASS] %s\n", name);
    }
    else
    {
        tests_failed++;
        printf("[FAIL] %s (Expected: %d, Got: %d)\n", name, expected, actual);
    }
}

void test_memory(void)
{
    printf("[Memory]\n");
    test_mem_primitives();
    test_heap_suite();
    test_pmm_suite();
    test_paging_suite();
    printf("Memory tests complete.\n");
}

void test_all(void)
{
    test_reset_counters();

    printf("\n");
    printf("========================================\n");
    printf("      Bare Minimum OS Test Suite\n");
    printf("========================================\n\n");

    uint32_t start = get_ticks();

    test_string();
    test_stdlib();
    test_memory();
    test_scheduler();
    test_user_suite();

    uint32_t end = get_ticks();
    int passed = test_get_passed();
    int failed = test_get_failed();

    printf("\n========================================\n");
    printf("Total  : %d\n", passed + failed);
    printf("Passed : %d\n", passed);
    printf("Failed : %d\n", failed);
    printf("Completed in %d ticks (%d ms)\n", end - start, ticks_to_ms(end - start));

    if (failed == 0)
        printf("Status : ALL TESTS PASSED\n");
    else
        printf("Status : SOME TESTS FAILED\n");

    printf("========================================\n");
}
/*
 * Kernel Test Suite
 *
 * Provides a lightweight unit testing framework for validating
 * core kernel components.
 */

#include "kernel_tests.h"
#include "../drivers/vga.h"
#include "../libc/mem.h"
#include "../libc/string.h"
#include "../libc/stdlib.h"
#include "../mm/heap.h"
#include "../mm/pmm.h"
#include "../mm/paging.h"
#include "../drivers/pit.h"

static int tests_passed = 0;
static int tests_failed = 0;

static void assert_true(int cond, const char *name);
static void assert_equal_int(int expected, int actual, const char *name);

/* string.h Tests */
static void test_strlen(void);
static void test_strcmp(void);
static void test_strncmp(void);

 /* stdlib.h Tests */
static void test_atoi(void);
static void test_itoa(void);
static void test_xtoi(void);
static void test_itox(void);

/* mem.h Tests */
static void test_memset(void);
static void test_memcpy(void);
static void test_heap(void);
static void test_pmm(void);
static void test_paging(void);

static void assert_true(int cond, const char *name)
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

static void assert_equal_int(int expected, int actual, const char *name)
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

/* Test Runner */
void test_all(void)
{
    tests_passed = 0;
    tests_failed = 0;
    
    printf("\n");
    printf("========================================\n");
    printf("      Bare Minimum OS Test Suite\n");
    printf("========================================\n\n");
    
    uint32_t start = get_ticks();

    test_string();
    test_stdlib();
    test_memory();

    uint32_t end = get_ticks();
    printf("\n========================================\n");
    printf("Total  : %d\n", tests_passed + tests_failed);
    printf("Passed : %d\n", tests_passed);
    printf("Failed : %d\n", tests_failed);
    printf("Completed in %d ticks (%d ms)\n", end - start, ticks_to_ms(end - start));

    if (tests_failed == 0)
        printf("Status : ALL TESTS PASSED\n");
    else
        printf("Status : SOME TESTS FAILED\n");

    printf("========================================\n");
}

void test_string(void)
{
    printf("[String Library]\n");
    test_strlen();
    test_strcmp();
    test_strncmp();
    printf("String tests complete.\n");
}

void test_stdlib(void)
{
    printf("[Standard Library]\n");
    test_atoi();
    test_itoa();
    test_xtoi();
    test_itox();
    printf("Stdlib tests complete.\n");
}

void test_memory(void)
{
    printf("[Memory]\n");
    test_memset();
    test_memcpy();
    test_heap();
    test_pmm();
    test_paging();
    printf("Memory tests complete.\n");
}

 /* string.h Tests */

static void test_strlen(void)
{
    assert_equal_int(0, strlen(""), "strlen(empty)");
    assert_equal_int(1, strlen("A"), "strlen(single)");
    assert_equal_int(5, strlen("Hello"), "strlen(normal)");
    assert_equal_int(11, strlen("Hello World"), "strlen(sentence)");
}

static void test_strcmp(void)
{
    assert_true(strcmp("abc", "abc") == 0, "strcmp(equal)");
    assert_true(strcmp("abc", "abd") < 0, "strcmp(less)");
    assert_true(strcmp("abd", "abc") > 0, "strcmp(greater)");
    assert_true(strcmp("", "") == 0, "strcmp(empty)");
}

static void test_strncmp(void)
{
    assert_true(strncmp("abcdef", "abcxyz", 3) == 0, "strncmp(first3)");
    assert_true(strncmp("abc", "abd", 3) < 0, "strncmp(diff)");
    assert_true(strncmp("abc", "abc", 0) == 0, "strncmp(n0)");
}

/* stdlib.h Tests */

static void test_atoi(void)
{
    assert_equal_int(123, atoi("123"), "atoi(positive)");
    assert_equal_int(-123, atoi("-123"), "atoi(negative)");
    assert_equal_int(0, atoi("0"), "atoi(zero)");
    assert_equal_int(42, atoi("    42"), "atoi(spaces)");
    assert_equal_int(42, atoi("+42"), "atoi(plus)");
    assert_equal_int(23, atoi("23abc"), "atoi(stop_non_digit)");
}

static void test_itoa(void)
{
    char buf[32];

    itoa(0, buf); assert_true(strcmp(buf, "0") == 0, "itoa(zero)");
    itoa(12345, buf); assert_true(strcmp(buf, "12345") == 0, "itoa(positive)");
    itoa(-6789, buf); assert_true(strcmp(buf, "-6789") == 0, "itoa(negative)");
}

static void test_xtoi(void)
{
    assert_equal_int(0, xtoi("0"), "xtoi(zero)");
    assert_equal_int(15, xtoi("F"), "xtoi(F)");
    assert_equal_int(255, xtoi("FF"), "xtoi(FF)");
    assert_equal_int(0xABCD, xtoi("0xABCD"), "xtoi(prefix)");
    assert_equal_int(0xDEAD, xtoi("dead"), "xtoi(lowercase)");
}

static void test_itox(void)
{
    char buf[32];
    itox(0, buf); assert_true(strcmp(buf, "0") == 0, "itox(zero)");
    itox(15, buf); assert_true(strcmp(buf, "F") == 0, "itox(F)");
    itox(255, buf); assert_true(strcmp(buf, "FF") == 0, "itox(FF)");
    itox(0xDEADBEEF, buf); assert_true(strcmp(buf, "DEADBEEF") == 0, "itox(DEADBEEF)");
}

/* mem.h Tests */
static void test_memset(void)
{
    char buf[16];

    memset(buf, 0, sizeof(buf));
    int ok = 1;

    for(int i = 0; i < 16; ++i)
    {
        if(buf[i] != 0)
        {
            ok = 0;
            break;
        }
    }
    assert_true(ok, "memset(zero)");

    memset(buf, 'A', sizeof(buf));
    ok = 1;

    for(int i = 0; i < 16; ++i)
    {
        if(buf[i] != 'A')
        {
            ok = 0;
            break;
        }
    }
    assert_true(ok, "memset(fill)");
}

static void test_memcpy(void)
{
    char src[] = "Hello";
    char dst[16];

    memset(dst, 0, sizeof(dst));
    memcpy(dst, src, strlen(src) + 1);
    assert_true(strcmp(src, dst) == 0, "memcpy(string)");

    int a[4] = {1,2,3,4};
    int b[4] = {0};

    memcpy(b, a, sizeof(a));
    int ok = 1;

    for(int i = 0; i < 4; ++i)
    {
        if(b[i] != a[i])
        {
            ok = 0;
            break;
        }
    }
    assert_true(ok, "memcpy(array)");
}

/* heap.h Tests */
void test_heap(void)
{
    printf("[Kernel Heap]\n");

    void *a = kmalloc(32);
    void *b = kmalloc(64);

    assert_true(a != 0, "kmalloc(32)");
    assert_true(b != 0, "kmalloc(64)");
    assert_true(a != b, "kmalloc(unique)");

    memset(a, 0xAA, 32);
    memset(b, 0xBB, 64);
    assert_equal_int(0xAA, ((uint8_t*)a)[0], "kmalloc(no overlap)");

    kfree(a);
    kfree(b);

    printf("Heap tests complete.\n");
}

/* pmm.h Tests */
void test_pmm(void)
{
    printf("[Physical Memory Manager]\n");

    void *a = pmm_alloc_block();
    void *b = pmm_alloc_block();

    assert_true(a != 0, "pmm(first)");
    assert_true(b != 0, "pmm(second)");
    assert_true(a != b, "pmm(unique)");

    assert_true(((uint32_t)a % 4096) == 0, "pmm(aligned)");

    pmm_free_block(a);
    pmm_free_block(b);

    printf("PMM tests complete.\n");
}

/* paging.h Tests */
void test_paging(void)
{
    printf("[Paging]\n");
    uint32_t phys = (uint32_t)pmm_alloc_block();

    map_page(0x20000000, phys);
    assert_equal_int(phys, get_physical_addr(0x20000000), "paging(map)");
    assert_equal_int(phys + 100, get_physical_addr(0x20000064), "paging(offset)");
    unmap_page(0x20000000);
    assert_equal_int(0, get_physical_addr(0x20000000), "paging(unmap)");

    pmm_free_block((void*)phys);
    printf("Paging tests complete.\n");
}
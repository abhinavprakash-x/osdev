#include "kernel_tests.h"
#include "../libc/mem.h"
#include "../libc/stdlib.h"
#include "../libc/string.h"
#include "../drivers/vga.h"

static int tests_passed = 0;
static int tests_failed = 0;

static void assert_true(int cond);
static void assert_equal_int(int expected, int actual);

/* string.h unit tests */
static void test_strlen(void);
static void test_strcmp(void);
static void test_strncmp(void);
/* stdlib.h unit tests */
static void test_atoi();
static void test_itoa();
static void test_xtoi();
static void test_itox();

void test_all(void)
{
    tests_passed = 0;
    tests_failed = 0;

    printf("\n======= Bare Minimum OS Test Suite =======\n\n");

    test_string();
    test_stdlib();

    printf("\n=======================================\n");
    printf("Total  : %d\n", tests_passed + tests_failed);
    printf("Passed : %d\n", tests_passed);
    printf("Failed : %d\n", tests_failed);

    if (tests_failed == 0)
        printf("Status : ALL TESTS PASSED\n");
    else
        printf("Status : SOME TESTS FAILED\n");

    printf("=======================================\n");
}

static void assert_true(int cond)
{
    if(cond) tests_passed++;
    else tests_failed++;
}

static void assert_equal_int(int expected, int actual)
{
    if(expected == actual) tests_passed++;
    else tests_failed++;
}

void test_string(void)
{
    printf("Testing Strings...\n");
    test_strlen();
    test_strcmp();
    test_strncmp();
}

void test_stdlib(void)
{
    printf("Testing stdlib...\n");
    test_atoi();
    test_itoa();
    test_xtoi();
    test_itox();
}

static void test_strlen(void)
{
    assert_equal_int(0, strlen(""));
    assert_equal_int(1, strlen("A"));
    assert_equal_int(5, strlen("Hello"));
    assert_equal_int(11, strlen("Hello World"));
}

static void test_strcmp(void)
{
    assert_true(strcmp("abc", "abc") == 0);
    assert_true(strcmp("abc", "abd") < 0);
    assert_true(strcmp("abd", "abc") > 0);
    assert_true(strcmp("", "") == 0);
}

static void test_strncmp(void)
{
    assert_true(strncmp("abcdef", "abcxyz", 3) == 0);
    assert_true(strncmp("abc", "abd", 3) < 0);
    assert_true(strncmp("abc", "abc", 0) == 0);
}

static void test_atoi(void)
{
    assert_equal_int(123, atoi("123"));
    assert_equal_int(-123, atoi("-123"));
    assert_equal_int(0, atoi("0"));
    assert_equal_int(42, atoi("    42"));
    assert_equal_int(42, atoi("+42"));
    assert_equal_int(23, atoi("23abc"));
}

static void test_itoa(void)
{
    char buf[32];
    itoa(0, buf); assert_true(strcmp(buf, "0") == 0);
    itoa(12345, buf); assert_true(strcmp(buf, "12345") == 0);
    itoa(-6789, buf); assert_true(strcmp(buf, "-6789") == 0);
}

static void test_xtoi(void)
{
    assert_equal_int(0, xtoi("0"));
    assert_equal_int(15, xtoi("F"));
    assert_equal_int(255, xtoi("FF"));
    assert_equal_int(0xABCD, xtoi("0xABCD"));
    assert_equal_int(0xdead, xtoi("dead"));
}

static void test_itox(void)
{
    char buf[32];
    itox(0, buf); assert_true(strcmp(buf, "0") == 0);
    itox(15, buf); assert_true(strcmp(buf, "F") == 0);
    itox(255, buf); assert_true(strcmp(buf, "FF") == 0);
    itox(0xDEADBEEF, buf); assert_true(strcmp(buf, "DEADBEEF") == 0);
}
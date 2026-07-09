#include "../libc/mem.h"
#include "../libc/stdlib.h"
#include "../libc/string.h"
#include "../drivers/vga.h"

static int tests_passed = 0;
static int tests_failed = 0;

static void assert_true(int cond, const char *name);
static void assert_equal_int(int expected, int actual, const char *name);

static void test_strlen(void);
static void test_strcmp(void);
static void test_strncmp(void);

void test_all(void)
{
    tests_passed = 0;
    tests_failed = 0;

    printf("\n======= Bare Minimum OS Test Suite =======\n\n");

    test_strlen();
    test_strcmp();
    test_strncmp();

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


static void assert_true(int cond, const char *name)
{
    if(cond)
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
    if(expected == actual)
    {
        tests_passed++;
        printf("[PASS] %s\n", name);
    }
    else
    {
        tests_failed++;
        printf("[FAIL] %s (Expected: %d, Got: %d)\n",
               name, expected, actual);
    }
}

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
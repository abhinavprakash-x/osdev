/*
 * Libc Unit Tests
 * Validates string.h, stdlib.h, and mem.h primitives.
 */

#include "apps/kernel_tests.h"
#include "tests/test_framework.h"
#include "libc/string.h"
#include "libc/stdlib.h"
#include "libc/mem.h"

static void test_strlen(void)
{
    test_assert_equal_int(0, (int)strlen(""), "strlen(empty)");
    test_assert_equal_int(1, (int)strlen("A"), "strlen(single)");
    test_assert_equal_int(5, (int)strlen("Hello"), "strlen(normal)");
    test_assert_equal_int(11, (int)strlen("Hello World"), "strlen(sentence)");
}

static void test_strcmp(void)
{
    test_assert_true(strcmp("abc", "abc") == 0, "strcmp(equal)");
    test_assert_true(strcmp("abc", "abd") < 0, "strcmp(less)");
    test_assert_true(strcmp("abd", "abc") > 0, "strcmp(greater)");
    test_assert_true(strcmp("", "") == 0, "strcmp(empty)");
}

static void test_strncmp(void)
{
    test_assert_true(strncmp("abcdef", "abcxyz", 3) == 0, "strncmp(first3)");
    test_assert_true(strncmp("abc", "abd", 3) < 0, "strncmp(diff)");
    test_assert_true(strncmp("abc", "abc", 0) == 0, "strncmp(n0)");
}

static void test_atoi(void)
{
    test_assert_equal_int(123, atoi("123"), "atoi(positive)");
    test_assert_equal_int(-123, atoi("-123"), "atoi(negative)");
    test_assert_equal_int(0, atoi("0"), "atoi(zero)");
    test_assert_equal_int(42, atoi("    42"), "atoi(spaces)");
    test_assert_equal_int(42, atoi("+42"), "atoi(plus)");
    test_assert_equal_int(23, atoi("23abc"), "atoi(stop_non_digit)");
}

static void test_itoa(void)
{
    char buf[32];

    itoa(0, buf);
    test_assert_true(strcmp(buf, "0") == 0, "itoa(zero)");

    itoa(12345, buf);
    test_assert_true(strcmp(buf, "12345") == 0, "itoa(positive)");

    itoa(-6789, buf);
    test_assert_true(strcmp(buf, "-6789") == 0, "itoa(negative)");

    itoa(-2147483648, buf);
    test_assert_true(strcmp(buf, "-2147483648") == 0, "itoa(INT_MIN)");
}

static void test_xtoi(void)
{
    test_assert_equal_int(0, (int)xtoi("0"), "xtoi(zero)");
    test_assert_equal_int(15, (int)xtoi("F"), "xtoi(F)");
    test_assert_equal_int(255, (int)xtoi("FF"), "xtoi(FF)");
    test_assert_equal_int(0xABCD, (int)xtoi("0xABCD"), "xtoi(prefix)");
    test_assert_equal_int(0xDEAD, (int)xtoi("dead"), "xtoi(lowercase)");
}

static void test_itox(void)
{
    char buf[32];

    itox(0, buf);
    test_assert_true(strcmp(buf, "0") == 0, "itox(zero)");

    itox(15, buf);
    test_assert_true(strcmp(buf, "F") == 0, "itox(F)");

    itox(255, buf);
    test_assert_true(strcmp(buf, "FF") == 0, "itox(FF)");

    itox(0xDEADBEEF, buf);
    test_assert_true(strcmp(buf, "DEADBEEF") == 0, "itox(DEADBEEF)");
}

static void test_memset(void)
{
    char buf[16];

    memset(buf, 0, sizeof(buf));
    int ok = 1;
    for (int i = 0; i < 16; ++i)
    {
        if (buf[i] != 0)
        {
            ok = 0;
            break;
        }
    }
    test_assert_true(ok, "memset(zero)");

    memset(buf, 'A', sizeof(buf));
    ok = 1;
    for (int i = 0; i < 16; ++i)
    {
        if (buf[i] != 'A')
        {
            ok = 0;
            break;
        }
    }
    test_assert_true(ok, "memset(fill)");
}

static void test_memcpy(void)
{
    char src[] = "Hello";
    char dst[16];

    memset(dst, 0, sizeof(dst));
    memcpy(dst, src, strlen(src) + 1);
    test_assert_true(strcmp(src, dst) == 0, "memcpy(string)");

    int a[4] = {1, 2, 3, 4};
    int b[4] = {0};

    memcpy(b, a, sizeof(a));
    int ok = 1;
    for (int i = 0; i < 4; ++i)
    {
        if (b[i] != a[i])
        {
            ok = 0;
            break;
        }
    }
    test_assert_true(ok, "memcpy(array)");
}

static void test_parse_hex_u32(void)
{
    uint32_t val = 0;

    test_assert_true(parse_hex_u32("0", &val) && val == 0, "parse_hex_u32(zero)");
    test_assert_true(parse_hex_u32("0xDEADBEEF", &val) && val == 0xDEADBEEFu, "parse_hex_u32(0xDEADBEEF)");
    test_assert_true(parse_hex_u32("00012345678", &val) && val == 0x12345678u, "parse_hex_u32(leading zeros)");

    // BUG-013: Reject > 8 hex digits (e.g. 123456789), empty strings, and non-hex characters
    test_assert_true(!parse_hex_u32("123456789", &val), "parse_hex_u32 BUG-013(>8 hex digits rejected)");
    test_assert_true(!parse_hex_u32("", &val), "parse_hex_u32 BUG-013(empty rejected)");
    test_assert_true(!parse_hex_u32("0x", &val), "parse_hex_u32 BUG-013(prefix only rejected)");
    test_assert_true(!parse_hex_u32("0x123G", &val), "parse_hex_u32 BUG-013(invalid char rejected)");
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
    test_parse_hex_u32();
    printf("Stdlib tests complete.\n");
}

void test_mem_primitives(void)
{
    test_memset();
    test_memcpy();
}
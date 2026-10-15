#pragma once
#include <stdint.h>
#include <stdbool.h>

void test_assert_true(int cond, const char *name);
void test_assert_equal_int(int expected, int actual, const char *name);

int test_get_passed(void);
int test_get_failed(void);
void test_reset_counters(void);

/* Sub-suite runners */
void test_mem_primitives(void);
void test_pmm_suite(void);
void test_paging_suite(void);
void test_heap_suite(void);
void test_user_suite(void);
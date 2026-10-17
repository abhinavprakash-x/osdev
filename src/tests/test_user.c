/*
 * Ring 3 User-Mode & Syscall Test Launcher
 * Covers BUG-002, BUG-003, BUG-004, BUG-010, BUG-017, and BUG-040.
 */

#include "apps/kernel_tests.h"
#include "tests/test_framework.h"
#include "mm/pmm.h"
#include "mm/paging.h"
#include "task/task.h"
#include "task/scheduler.h"
#include "libc/mem.h"
#include "libc/stdlib.h"

extern const uint8_t user_test_start[];
extern const uint32_t user_test_size;

void test_user_suite(void)
{
    printf("[User Payload & Task Creation]\n");

    test_assert_true(user_test_size > 0, "user_test_size(non-empty)");
    test_assert_true(user_test_size <= PAGE_SIZE, "user_test_size(fits in single 4KB page)");

    int used_before = get_used_memory();

    // BUG-010: Reject null buffer, zero size, or size > PAGE_SIZE
    test_assert_true(create_user_task("u_null", 0, user_test_size, USER_CODE_LOAD_ADDR, USER_STACK_TOP_ADDR) == 0,
                     "user task BUG-010(null code rejected)");
    test_assert_true(create_user_task("u_zero", user_test_start, 0, USER_CODE_LOAD_ADDR, USER_STACK_TOP_ADDR) == 0,
                     "user task BUG-010(zero size rejected)");
    test_assert_true(create_user_task("u_oversize", user_test_start, PAGE_SIZE + 1, USER_CODE_LOAD_ADDR, USER_STACK_TOP_ADDR) == 0,
                     "user task BUG-010(>4KB payload rejected)");

    // BUG-003 & BUG-004: Force mapping failure (stack in shared kernel PDE 0 at 0x00100000, or colliding with code page)
    // and verify complete resource rollback without leaking PMM frames or heap blocks.
    task_t* bad_code_map = create_user_task("u_bad_code", user_test_start, user_test_size, 0x00100000u, USER_STACK_TOP_ADDR);
    test_assert_true(bad_code_map == 0, "user task BUG-004(shared kernel PDE code map rejected)");

    task_t* bad_stack_map = create_user_task("u_bad_stack", user_test_start, user_test_size, USER_CODE_LOAD_ADDR, USER_CODE_LOAD_ADDR + PAGE_SIZE);
    test_assert_true(bad_stack_map == 0, "user task BUG-004(colliding code/stack map rejected)");
    test_assert_equal_int(used_before, get_used_memory(), "user task BUG-003(failed creation leaks 0 frames)");

    // BUG-002 & BUG-017: Verify created user task has mapped code & stack and copied payload bytes
    task_t* utask = create_user_task("u_valid", user_test_start, user_test_size, USER_CODE_LOAD_ADDR, USER_STACK_TOP_ADDR);
    test_assert_true(utask != 0, "user task BUG-002(create valid user task)");
    if (utask != 0)
    {
        uint32_t eflags;
        __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

        paging_switch_directory(utask->page_directory);
        bool code_mapped  = user_range_valid(USER_CODE_LOAD_ADDR, user_test_size);
        bool stack_mapped = user_range_valid(USER_STACK_TOP_ADDR - PAGE_SIZE, PAGE_SIZE);
        bool code_copied  = (*(const uint32_t*)USER_CODE_LOAD_ADDR == *(const uint32_t*)user_test_start);
        paging_switch_directory(paging_get_kernel_directory());

        __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");

        test_assert_true(code_mapped, "user task BUG-002(code page mapped in user space)");
        test_assert_true(stack_mapped, "user task BUG-002(stack page mapped in user space)");
        test_assert_true(code_copied, "user task BUG-017(payload copied via temp mapping)");

        destroy_task(utask);
    }

    test_assert_equal_int(used_before, get_used_memory(), "user task(cleanup restores PMM frames)");
}

void launch_user_test(void)
{
    printf("[User Mode / Syscalls]\n");

    task_t* user_task = create_user_task(
        "user_test",
        user_test_start,
        user_test_size,
        USER_CODE_LOAD_ADDR,
        USER_STACK_TOP_ADDR
    );

    if (user_task == 0)
    {
        printf("[FAIL] Could not create user task\n");
        return;
    }

    task_add(user_task);
    printf("[ OK ] User test task created\n");
}
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
#include "drivers/pit.h"

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

    static const uint8_t crashing_user_code[4] = { 0xFA, 0xEB, 0xFE, 0x90 }; // cli; jmp $
    task_t* crash_task = create_user_task("u_crash", crashing_user_code, sizeof(crashing_user_code),
                                        USER_CODE_LOAD_ADDR, USER_STACK_TOP_ADDR);
    test_assert_true(crash_task != 0, "user task BUG-049(create crashing user task)");
    if (crash_task != 0)
    {
        uint32_t crash_pid = crash_task->pid;
        task_add(crash_task);

        uint32_t start_ticks = get_ticks();
        uint32_t timeout = get_timer_freq();
        if (timeout == 0) timeout = 100;

        while (scheduler_pid_exists(crash_pid) && (get_ticks() - start_ticks) < timeout)
        {
            yield();
        }
        yield(); // Ensure reap_dead_tasks() frees the terminated task

        test_assert_true(!scheduler_pid_exists(crash_pid), "user task BUG-049(crashing Ring 3 task terminated & reaped)");
        test_assert_equal_int(used_before, get_used_memory(), "user task BUG-049(crashing task frames reclaimed)");

        int32_t crash_status = 0;
        test_assert_true(task_get_exit_status(crash_pid, &crash_status) && crash_status == -141,
                         "user task Milestone3(crash exit status recorded as -141)");
    }

    // Verify Ring 3 SYS_EXIT records exit status from EBX and task_wait_pid() retrieves it
    // Machine code: mov ebx, 77 (BB 4D 00 00 00); mov eax, 2 (B8 02 00 00 00); int 0x80 (CD 80)
    static const uint8_t exit77_user_code[12] = {
        0xBB, 0x4D, 0x00, 0x00, 0x00,
        0xB8, 0x02, 0x00, 0x00, 0x00,
        0xCD, 0x80
    };
    task_t* exit_task = create_user_task("u_exit77", exit77_user_code, sizeof(exit77_user_code),
                                         USER_CODE_LOAD_ADDR, USER_STACK_TOP_ADDR);
    test_assert_true(exit_task != 0, "user task Milestone3(create exit77 user task)");
    if (exit_task != 0)
    {
        uint32_t exit_pid = exit_task->pid;
        int32_t status = 0;
        task_add(exit_task);

        bool waited = task_wait_pid(exit_pid, &status);
        test_assert_true(waited, "user task Milestone3(task_wait_pid succeeded)");
        test_assert_equal_int(77, (int)status, "user task Milestone3(SYS_EXIT status 77 retrieved)");
        test_assert_equal_int(used_before, get_used_memory(), "user task Milestone3(PMM frames restored)");
    }
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
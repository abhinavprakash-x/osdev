/*
 * Ring 3 User-Mode & Syscall Test Launcher
 * Uses assemble-time `user_test_size` symbol (BUG-040 fix).
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
    printf("[User Payload Image]\n");
    test_assert_true(user_test_size > 0, "user_test_size(non-empty)");
    test_assert_true(user_test_size <= PAGE_SIZE, "user_test_size(fits in single 4KB page)");
}

void launch_user_test(void)
{
    printf("[User Mode / Syscalls]\n");

    uint32_t user_code_size = user_test_size;
    if (user_code_size == 0 || user_code_size > PAGE_SIZE)
    {
        printf("[FAIL] Invalid user test payload size: %d\n", user_code_size);
        return;
    }

    uint32_t user_code_phys = (uint32_t)pmm_alloc_block();
    if (!user_code_phys)
    {
        printf("[FAIL] Could not allocate user code page\n");
        return;
    }

    task_t* user_task = create_user_task("user_test", 0x400000, 0x800000 + 4096);
    if (user_task == 0)
    {
        printf("[FAIL] Could not create user task\n");
        pmm_free_block((void*)user_code_phys);
        return;
    }

    memcpy((void*)user_code_phys, user_test_start, user_code_size);
    uint32_t user_stack_phys = (uint32_t)pmm_alloc_block();

    if (!user_stack_phys)
    {
        printf("[FAIL] Could not allocate user stack\n");
        pmm_free_block((void*)user_code_phys);
        return;
    }

    paging_map_page(user_task->page_directory, 0x400000, user_code_phys, PTE_PRESENT | PTE_USER);
    paging_map_page(user_task->page_directory, 0x800000, user_stack_phys, PTE_PRESENT | PTE_RW | PTE_USER);
    task_add(user_task);

    printf("[ OK ] User test task created\n");
}
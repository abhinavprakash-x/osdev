/*
 * Scheduler & Multitasking Tests
 * Covers BUG-006 (null task check), BUG-015 (scheduler_init PID 0 verification),
 * and BUG-038 (timing-independent task completion waiting).
 */

#include "apps/kernel_tests.h"
#include "tests/test_framework.h"
#include "task/task.h"
#include "task/scheduler.h"
#include "mm/paging.h"
#include "drivers/pit.h"
#include "libc/string.h"
#include "libc/stdlib.h"

static volatile uint32_t test_counter_a = 0;
static volatile uint32_t test_counter_b = 0;
static volatile int task_a_finished = 0;
static volatile int task_b_finished = 0;

static void scheduler_test_task_a(void)
{
    for (int i = 0; i < 5; i++)
    {
        test_counter_a++;
        task_sleep(20);
    }
    task_a_finished = 1;
    task_exit();
}

static void scheduler_test_task_b(void)
{
    for (int i = 0; i < 5; i++)
    {
        test_counter_b++;
        task_sleep(30);
    }
    task_b_finished = 1;
    task_exit();
}

/*
 * Regression test for BUG-015: verify that scheduler_init() properly
 * allocated and linked the initial PID 0 ("kmain") task context.
 */
static void test_scheduler_init_state(void)
{
    test_assert_true(current_task != 0, "scheduler init(current_task non-null)");
    if (current_task == 0) return;

    task_t* iter = current_task;
    task_t* kmain_task = 0;
    int safety_limit = 64;

    do
    {
        if (iter->pid == 0)
        {
            kmain_task = iter;
            break;
        }
        iter = iter->next;
    } while (iter != 0 && iter != current_task && --safety_limit > 0);

    test_assert_true(kmain_task != 0, "scheduler init(PID 0 present in ring)");
    if (kmain_task != 0)
    {
        test_assert_true(strcmp(kmain_task->name, "kmain") == 0, "scheduler init(PID 0 name is kmain)");
        test_assert_true(kmain_task->page_directory == paging_get_kernel_directory(), "scheduler init(PID 0 uses kernel page directory)");
        test_assert_true(kmain_task->next != 0, "scheduler init(circular link valid)");
    }
}

void test_scheduler(void)
{
    printf("[Scheduler]\n");

    test_scheduler_init_state();

    test_counter_a = 0;
    test_counter_b = 0;
    task_a_finished = 0;
    task_b_finished = 0;

    task_t* task_a = create_task("test_a", scheduler_test_task_a);
    task_t* task_b = create_task("test_b", scheduler_test_task_b);

    test_assert_true(task_a != 0, "Create scheduler task A");
    test_assert_true(task_b != 0, "Create scheduler task B");

    task_add(task_a);
    task_add(task_b);
    task_add(0); // BUG-006: NULL task insertion must be safely ignored

    /*
     * BUG-038 fix: Instead of assuming a hardcoded 100 Hz PIT rate and
     * fixed 250 ms completion window, poll with yield() until both tasks
     * finish or a generous frequency-scaled timeout (3 seconds) expires.
     */
    uint32_t freq = get_timer_freq();
    if (freq == 0) freq = 100;
    uint32_t timeout_ticks = freq * 3;
    uint32_t start_ticks = get_ticks();

    while ((!task_a_finished || !task_b_finished) &&
           (get_ticks() - start_ticks) < timeout_ticks)
    {
        yield();
    }

    // One additional yield so schedule() unlinks and frees the dead tasks
    yield();

    test_assert_equal_int(5, (int)test_counter_a, "Scheduler executed task A");
    test_assert_equal_int(5, (int)test_counter_b, "Scheduler executed task B");
    test_assert_true(task_a_finished && task_b_finished, "Scheduler tasks completed before timeout");

    printf("Scheduler tests complete.\n");
}
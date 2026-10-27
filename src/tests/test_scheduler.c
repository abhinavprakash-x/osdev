/*
 * Scheduler & Multitasking Tests
 * Covers BUG-005, BUG-006, BUG-014, BUG-015, BUG-028, BUG-038, and BUG-041.
 */

#include "apps/kernel_tests.h"
#include "tests/test_framework.h"
#include "task/task.h"
#include "task/scheduler.h"
#include "mm/paging.h"
#include "mm/pmm.h"
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

static void dummy_instant_exit_task(void)
{
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

/*
 * Regression test for BUG-028: 64-bit sleep tick conversion & 32-bit tick wraparound
 */
static void test_scheduler_sleep_wraparound_math(void)
{
    // 1. Pre-wraparound: now = 0xFFFFFFF0, wake_time = 0x00000010 (32 ticks later)
    test_assert_true(!ticks_reached(0xFFFFFFF0u, 0x00000010u), "scheduler BUG-028(pre-wrap not reached)");
    test_assert_true(ticks_reached(0x00000010u, 0x00000010u), "scheduler BUG-028(exact wrap target reached)");
    test_assert_true(ticks_reached(0x00000015u, 0x00000010u), "scheduler BUG-028(post-wrap target reached)");

    // 2. Large millisecond conversion must not truncate before / 1000
    uint32_t clamped = ms_to_ticks(0xFFFFFFFFu);
    test_assert_true(clamped > 0 && clamped <= MAX_SLEEP_TICKS, "scheduler BUG-028(large ms safely clamped)");
}

/*
 * Regression test for BUG-041: PID wraparound skips PID 0 and active PIDs
 */
static void test_scheduler_pid_wraparound(void)
{
    uint32_t saved_pid = task_get_next_pid();

    task_set_next_pid(0xFFFFFFFFu);
    task_t* t1 = create_task("pid_max", dummy_instant_exit_task);
    task_t* t2 = create_task("pid_wrap", dummy_instant_exit_task);

    test_assert_true(t1 != 0 && t2 != 0, "scheduler BUG-041(create wraparound tasks)");
    if (t1 != 0 && t2 != 0)
    {
        test_assert_true(t1->pid == 0xFFFFFFFFu, "scheduler BUG-041(assigns UINT32_MAX)");
        test_assert_true(t2->pid != 0 && t2->pid != current_task->pid, "scheduler BUG-041(skips PID 0 and active shell PID)");
    }

    if (t1 != 0) destroy_task(t1);
    if (t2 != 0) destroy_task(t2);
    task_set_next_pid(saved_pid);
}

/*
 * Regression test for BUG-014 & BUG-005:
 * - Multiple consecutive dead tasks reaped cleanly without use-after-free
 * - All-waiting ring with interrupts disabled (IF=0) wakes via idle hlt
 */
static void test_scheduler_dead_reap_and_all_waiting(void)
{
    task_t* d1 = create_task("dead_1", dummy_instant_exit_task);
    task_t* d2 = create_task("dead_2", dummy_instant_exit_task);
    task_t* d3 = create_task("dead_3", dummy_instant_exit_task);

    test_assert_true(d1 != 0 && d2 != 0 && d3 != 0, "scheduler BUG-014(create 3 short-lived tasks)");
    if (d1 != 0 && d2 != 0 && d3 != 0)
    {
        // Mark two tasks dead before scheduling and let the third exit via execution
        d1->state = TASK_DEAD;
        d2->state = TASK_DEAD;
        task_add(d1);
        task_add(d2);
        task_add(d3);

        uint32_t pid1 = d1->pid;
        uint32_t pid2 = d2->pid;
        uint32_t pid3 = d3->pid;

        yield();
        yield();

        test_assert_true(!scheduler_pid_exists(pid1) &&
                         !scheduler_pid_exists(pid2) &&
                         !scheduler_pid_exists(pid3),
                         "scheduler BUG-014(consecutive dead tasks reaped safely)");
    }

    // BUG-005: Force all tasks in the ring into TASK_WAITING while IF=0
    // and verify schedule() idles until the next tick instead of hanging.
    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    uint32_t wake_target = get_ticks() + 2;
    task_t* iter = current_task;
    do
    {
        iter->state = TASK_WAITING;
        iter->wake_time = wake_target;
        iter = iter->next;
    } while (iter != current_task);

    schedule();

    bool woke_up = ticks_reached(get_ticks(), wake_target) && (current_task->state == TASK_RUNNING);

    // Ensure all remaining tasks (e.g. kmain) are restored to TASK_READY
    iter = current_task->next;
    while (iter != current_task)
    {
        if (iter->state == TASK_WAITING)
            iter->state = TASK_READY;
        iter = iter->next;
    }

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");

    test_assert_true(woke_up, "scheduler BUG-005(all-waiting ring wakes with IF=0)");
}

void test_scheduler(void)
{
    printf("[Scheduler]\n");

    test_scheduler_init_state();
    test_scheduler_sleep_wraparound_math();
    test_scheduler_pid_wraparound();
    test_scheduler_dead_reap_and_all_waiting();

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

    uint32_t freq = get_timer_freq();
    if (freq == 0) freq = 100;
    uint32_t timeout_ticks = freq * 3;
    uint32_t start_ticks = get_ticks();

    while ((!task_a_finished || !task_b_finished) &&
           (get_ticks() - start_ticks) < timeout_ticks)
    {
        yield();
    }

    yield();

    test_assert_equal_int(5, (int)test_counter_a, "Scheduler executed task A");
    test_assert_equal_int(5, (int)test_counter_b, "Scheduler executed task B");
    test_assert_true(task_a_finished && task_b_finished, "Scheduler tasks completed before timeout");

    printf("Scheduler tests complete.\n");
}
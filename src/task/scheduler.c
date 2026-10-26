#include "task/scheduler.h"
#include "mm/heap.h"
#include "drivers/pit.h"
#include "libc/stdlib.h"
#include "cpu/tss.h"
#include "mm/paging.h"

task_t* current_task = 0;
static volatile int scheduler_idling = 0;

bool scheduler_is_idling(void)
{
    return scheduler_idling != 0;
}

bool scheduler_pid_exists(uint32_t pid)
{
    if (current_task == 0) return false;

    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    task_t* iter = current_task;
    bool found = false;
    do
    {
        if (iter->pid == pid && iter->state != TASK_DEAD)
        {
            found = true;
            break;
        }
        iter = iter->next;
    } while(iter != 0 && iter != current_task);

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
    return found;
}

void scheduler_init(void)
{
    // 1. Create PID 0 for the initial kernel execution context.
    // After initialization, kmain acts as the scheduler's idle task.
    task_t* main_task = (task_t*)kmalloc(sizeof(task_t));
    if(main_task == 0)
    {
        __asm__ volatile("cli; hlt");
    }

    main_task->esp = 0;
    main_task->pid = 0;
    main_task->name = "kmain";
    main_task->state = TASK_RUNNING;
    main_task->stack_allocation = 0;
    main_task->kernel_stack_top = 0;
    main_task->page_directory = paging_get_kernel_directory();
    main_task->wake_time = 0;

    // 2. Make it a circular linked list pointing to itself
    main_task->next = main_task;
    current_task = main_task;
}

void task_add(task_t* new_task)
{
    if (new_task == 0) return;
    if (current_task == 0) return;

    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    task_t* tail = current_task;
    while (tail->next != current_task) {
        tail = tail->next;
    }

    tail->next = new_task;
    new_task->next = current_task;

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
}

static void reap_dead_tasks(task_t* active_task)
{
    if (active_task == 0) return;

    task_t* prev = active_task;
    task_t* curr = active_task->next;

    while (curr != active_task)
    {
        if (curr->state == TASK_DEAD)
        {
            task_t* dead = curr;
            prev->next = curr->next;
            curr = prev->next;
            destroy_task(dead);
        }
        else
        {
            prev = curr;
            curr = curr->next;
        }
    }
}

void schedule(void)
{
    if (current_task == 0) return;

    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    task_t* previous_task = current_task;
    reap_dead_tasks(previous_task);

    // If the only remaining task in the entire ring is dead, halt the system
    if (previous_task->next == previous_task && previous_task->state == TASK_DEAD)
    {
        while (1) __asm__ volatile("cli; hlt");
    }

    task_t* next_task = 0;

    while (next_task == 0)
    {
        uint32_t now = get_ticks();
        task_t* iter = previous_task->next;

        do
        {
            if (iter->state == TASK_WAITING && ticks_reached(now, iter->wake_time))
            {
                iter->state = TASK_READY;
            }

            if (iter->state == TASK_READY || iter->state == TASK_RUNNING)
            {
                next_task = iter;
                break;
            }

            iter = iter->next;
        } while (iter != previous_task->next);

        if (next_task != 0)
            break;

        // Every live task in the ring is currently TASK_WAITING.
        scheduler_idling = 1;
        __asm__ volatile ("sti; hlt; cli" : : : "memory");
        scheduler_idling = 0;
    }

    if (previous_task->state == TASK_RUNNING)
        previous_task->state = TASK_READY;

    current_task = next_task;
    current_task->state = TASK_RUNNING;

    if (previous_task != current_task)
    {
        if (current_task->kernel_stack_top != 0)
            tss_set_kernel_stack(current_task->kernel_stack_top);
        else
            tss_set_kernel_stack(0xA0000);

        paging_switch_directory(current_task->page_directory);
        switch_task(&previous_task->esp, &current_task->esp);

        // When this task is later switched back in, reap any task that died
        // right before switching to us.
        reap_dead_tasks(current_task);
    }
    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
}

void yield(void)
{
    schedule();
}

void task_exit(void)
{
    if (current_task != 0) {
        current_task->state = TASK_DEAD;
    }

    yield();

    while (1)
    {
        __asm__ volatile("cli; hlt");
    }
}

void task_sleep(uint32_t milliseconds)
{
    if (current_task == 0) return;

    uint32_t ticks_to_wait = ms_to_ticks(milliseconds);
    if (ticks_to_wait == 0)
    {
        yield();
        return;
    }

    current_task->wake_time = get_ticks() + ticks_to_wait;
    current_task->state = TASK_WAITING;

    yield();
}

static const char* task_state_string(task_state_t state)
{
    switch (state)
    {
        case TASK_READY:   return "READY";
        case TASK_RUNNING: return "RUNNING";
        case TASK_WAITING: return "WAITING";
        case TASK_DEAD:    return "DEAD";
        default:           return "UNKNOWN";
    }
}

void print_tasks(void)
{
    if (current_task == 0)
    {
        printf("Scheduler not initialized.\n");
        return;
    }

    printf("PID\tNAME\tSTATE\n");
    printf("------------------------\n");

    task_t* task = current_task;

    do
    {
        printf("%d\t%s\t%s\n", task->pid, task->name, task_state_string(task->state));
        task = task->next;
    } while (task != current_task);
}
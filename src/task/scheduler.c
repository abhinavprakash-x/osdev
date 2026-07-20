#include "scheduler.h"
#include "../mm/heap.h"
#include "../drivers/pit.h"
#include "../libc/stdlib.h"

task_t* current_task = 0;

void scheduler_init(void)
{
    //1. Create PID 0 for the initial kernel execution context.
    // After initialization, kmain acts as the scheduler's idle task.
    task_t* main_task = (task_t*)kmalloc(sizeof(task_t));

    main_task->esp = 0;
    main_task->pid = 0;
    main_task->name = "kmain";
    main_task->state = TASK_RUNNING;
    main_task->stack_allocation = 0;
    main_task->wake_time = 0;
    main_task->next = main_task;
    
    // 2. Make it a circular linked list pointing to itself
    main_task->next = main_task;
    current_task = main_task;
}

void task_add(task_t* new_task)
{
    if (current_task == 0) return;

    // Traverse the circular list to find the last node
    // (The node whose 'next' pointer points back to current_task)
    task_t* tail = current_task;
    while (tail->next != current_task) {
        tail = tail->next;
    }

    // Insert the new task between the tail and the current_task
    tail->next = new_task;
    new_task->next = current_task;
}

void schedule(void)
{
    if (current_task == 0) return;

    // Edge case: If there is only one task and it died, the OS is out of things to do.
    if (current_task->next == current_task && current_task->state == TASK_DEAD) {
        // Kernel Panic: No runnable tasks
        __asm__ volatile("cli; hlt");
    }

    task_t* previous_task = current_task;
    task_t* prev_iterator = current_task;
    task_t* iterator = current_task->next;

    // Search the circular list for the next ready task
    while (1) 
    {
        if (iterator->state == TASK_DEAD) 
        {
            // Unlink the dead task from the list
            prev_iterator->next = iterator->next;
            
            kfree(iterator->stack_allocation);
            kfree(iterator);
            
            iterator = prev_iterator->next;
            continue;
        }

        if (iterator->state == TASK_WAITING) 
        {
            // Is it time to wake up?
            if (get_ticks() >= iterator->wake_time)
            {
                iterator->state = TASK_READY;
            }
            else
            {
                // Still sleeping, skip it
                prev_iterator = iterator;
                iterator = iterator->next;
                continue;
            }
        }

        if (iterator->state == TASK_READY || iterator->state == TASK_RUNNING) 
        {
            // We found a task to run!
            break;
        }
    }

    if (previous_task->state == TASK_RUNNING)
        previous_task->state = TASK_READY;

    current_task = iterator;
    current_task->state = TASK_RUNNING;

    if (previous_task != current_task)
    {
        switch_task(&previous_task->esp, &current_task->esp);
    }
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
    
    // Immediately hand the CPU to another task.
    // The scheduler loop will eventually see this task is dead, 
    // bypass it, and clean up its memory.
    yield();

    while (1)
    {
        __asm__ volatile("cli; hlt");
    }
}

void task_sleep(uint32_t milliseconds)
{
    if (current_task == 0) return;

    // Formula: (ms * frequency) / 1000
    uint32_t ticks_to_wait = (milliseconds * get_timer_freq() + 999) / 1000; 

    current_task->wake_time = get_ticks() + ticks_to_wait;
    current_task->state = TASK_WAITING;

    // Immediately hand over the CPU so other tasks can run while we wait
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
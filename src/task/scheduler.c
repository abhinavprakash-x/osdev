#include "scheduler.h"
#include "../mm/heap.h"

task_t* current_task = 0;

void scheduler_init(void)
{
    // 1. Create the dummy PCB for the currently running kmain thread
    task_t* main_task = (task_t*)kmalloc(sizeof(task_t));
    main_task->esp = 0; // The assembly stub will fill this in automatically on the first switch!
    
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
    if (current_task == 0 || current_task->next == current_task) {
        return;
    }

    task_t* previous_task = current_task;
    
    // Round-Robin: simply move to the next task in the circle
    current_task = current_task->next;

    // Call the assembly stub! 
    switch_task(&previous_task->esp, &current_task->esp);
}
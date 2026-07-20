#pragma once
#include <stdint.h>

typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_WAITING,
    TASK_DEAD
} task_state_t;

typedef struct task{
    uint32_t esp;
    uint32_t pid;
    task_state_t state;
    void* stack_allocation;
    uint32_t wake_time;
    struct task* next;
} task_t;

extern void switch_task(uint32_t* current_esp, uint32_t* next_esp);
task_t* create_task(void (*entry_point)(void));
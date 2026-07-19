#pragma once
#include <stdint.h>

typedef struct task{
    uint32_t esp;
    struct task* next;
} task_t;

extern void switch_task(uint32_t* current_esp, uint32_t* next_esp);
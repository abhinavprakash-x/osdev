#pragma once
#include "task.h"

void scheduler_init(void);
void task_add(task_t* new_task);
void schedule(void);
void yield(void);
void task_exit(void);
void sleep(uint32_t milliseconds);
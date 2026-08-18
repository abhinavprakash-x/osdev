#pragma once
#include "task.h"

extern task_t* current_task;

void scheduler_init(void);
void task_add(task_t* new_task);
void schedule(void);
void yield(void);
void task_exit(void);
void task_sleep(uint32_t milliseconds);
void print_tasks(void);
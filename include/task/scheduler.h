#pragma once
#include "task.h"
#include <stdbool.h>

extern task_t* current_task;

void scheduler_init(void);
void task_add(task_t* new_task);

void schedule(void);
void yield(void);

void task_exit(void);
void task_exit_with_status(int32_t status);
void task_sleep(uint32_t milliseconds);

bool task_get_exit_status(uint32_t pid, int32_t* out_status);
bool task_wait_pid(uint32_t pid, int32_t* out_status);

void print_tasks(void);
bool scheduler_pid_exists(uint32_t pid);
bool scheduler_is_idling(void);
#pragma once
#include <stdint.h>

#define USER_CODE_LOAD_ADDR 0x00400000u
#define USER_STACK_TOP_ADDR 0x00801000u

typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_WAITING,
    TASK_DEAD
} task_state_t;

typedef struct task{
    uint32_t esp;
    uint32_t pid;
    const char* name;
    task_state_t state;
    int32_t exit_status;

    void* stack_allocation;
    uint32_t kernel_stack_top;
    uint32_t* page_directory;

    uint32_t wake_time;
    struct task* next;
} task_t;

extern void switch_task(uint32_t* current_esp, uint32_t* next_esp);
task_t* create_task(const char* name, void (*entry_point)(void));
task_t* create_user_task(const char* name, const void* code, uint32_t code_size, uint32_t user_eip, uint32_t user_esp);
void destroy_task(task_t* task);

uint32_t task_get_next_pid(void);
void task_set_next_pid(uint32_t next);
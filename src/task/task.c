#include "task.h"
#include "../mm/heap.h"
#include "scheduler.h"

extern void enter_usermode(uint32_t user_eip, uint32_t user_esp);
static uint32_t next_pid = 1;

task_t* create_task(const char* name, void (*entry_point)(void))
{
    // 1. Allocate the PCB and a 4KB stack from the heap
    task_t* new_task = (task_t*)kmalloc(sizeof(task_t));
    void* stack_memory = kmalloc(4096);
    
    // 2. Point to the very top of the newly allocated 4KB stack
    uint32_t* stack = (uint32_t*)((uint8_t*)stack_memory + 4096);

    // 3. Forge the stack (walking downwards)
    
    // Fake return address for the C function
    *(--stack) = (uint32_t)task_exit; 
    
    // EIP: The address of the function we want this task to execute
    *(--stack) = (uint32_t)entry_point; 
    
    // EFLAGS: 0x202 sets the Interrupt Enable Flag (IF)
    // This ensures interrupts turn back on the moment this task starts!
    *(--stack) = 0x202; 
    
    // pushad pushes 8 registers. We forge 8 zeroes for popad to consume.
    *(--stack) = 0; // EAX
    *(--stack) = 0; // ECX
    *(--stack) = 0; // EDX
    *(--stack) = 0; // EBX
    *(--stack) = 0; // ESP
    *(--stack) = 0; // EBP
    *(--stack) = 0; // ESI
    *(--stack) = 0; // EDI

    // 4. Save the current stack pointer into the PCB
    new_task->esp = (uint32_t)stack;
    new_task->next = 0;
    new_task->pid = next_pid++;
    new_task->name = name;
    new_task->state = TASK_READY;
    new_task->stack_allocation = stack_memory; // CRITICAL for kfree later!
    new_task->kernel_stack_top = 0;
    new_task->wake_time = 0;

    return new_task;
}

task_t* create_user_task(const char* name, uint32_t user_eip, uint32_t user_esp)
{
    task_t* new_task = (task_t*)kmalloc(sizeof(task_t));
    void* stack_memory = kmalloc(4096);
    
    uint32_t* stack = (uint32_t*)((uint8_t*)stack_memory + 4096);

    // CRITICAL: Save the top of the stack for the TSS so interrupts don't crash!
    new_task->kernel_stack_top = (uint32_t)stack;

    // 1. Forge arguments for enter_usermode (matches [esp + 4] and [esp + 8])
    *(--stack) = user_esp;            
    *(--stack) = user_eip;            
    
    // 2. Fake return address for the C calling convention
    *(--stack) = (uint32_t)task_exit; 
    
    // 3. EIP: We want switch_task to 'ret' into enter_usermode!
    *(--stack) = (uint32_t)enter_usermode; 
    
    *(--stack) = 0x202; // EFLAGS
    
    // 4. pushad zeroes
    *(--stack) = 0; // EAX
    *(--stack) = 0; // ECX
    *(--stack) = 0; // EDX
    *(--stack) = 0; // EBX
    *(--stack) = 0; // ESP (Ignored by popad)
    *(--stack) = 0; // EBP
    *(--stack) = 0; // ESI
    *(--stack) = 0; // EDI

    new_task->esp = (uint32_t)stack;
    new_task->next = 0;
    new_task->pid = next_pid++; // Note: ensure next_pid is accessible
    new_task->name = name;
    new_task->state = TASK_READY;
    new_task->stack_allocation = stack_memory; 
    new_task->wake_time = 0;

    return new_task;
}
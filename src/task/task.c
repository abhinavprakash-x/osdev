#include "task.h"
#include "../mm/heap.h"

// A safety net in case a kernel thread accidentally returns.
static void task_exit(void) 
{
    while(1) {
        __asm__ volatile("cli; hlt");
    }
}

task_t* create_task(void (*entry_point)(void))
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

    return new_task;
}
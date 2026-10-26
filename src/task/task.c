#include "task/task.h"
#include "task/scheduler.h"
#include "mm/heap.h"
#include "mm/paging.h"
#include "mm/pmm.h"
#include "libc/mem.h"

extern void enter_usermode(uint32_t user_eip, uint32_t user_esp);
static uint32_t next_pid = 1;

static uint32_t allocate_pid(void)
{
    for (uint32_t attempts = 0; attempts < 0xFFFFFFFFu; ++attempts)
    {
        if (next_pid == 0)
            next_pid = 1;

        uint32_t candidate = next_pid++;
        if (!scheduler_pid_exists(candidate))
            return candidate;
    }
    return 0;
}

uint32_t task_get_next_pid(void)
{
    return next_pid;
}

void task_set_next_pid(uint32_t next)
{
    next_pid = (next == 0) ? 1 : next;
}

void destroy_task(task_t* task)
{
    if (task == 0) return;

    if (task->page_directory != 0 && task->page_directory != paging_get_kernel_directory())
    {
        paging_destroy_address_space(task->page_directory);
    }
    if (task->stack_allocation != 0)
    {
        kfree(task->stack_allocation);
    }
    kfree(task);
}

task_t* create_task(const char* name, void (*entry_point)(void))
{
    if (name == 0 || entry_point == 0) return 0;

    // 1. Allocate the PCB and a 4KB stack from the heap
    task_t* new_task = (task_t*)kmalloc(sizeof(task_t));
    if (new_task == 0) return 0;

    void* stack_memory = kmalloc(4096);
    if (stack_memory == 0)
    {
        kfree(new_task);
        return 0;
    }

    // 2. Point to the very top of the newly allocated 4KB stack
    uint32_t* stack = (uint32_t*)((uint8_t*)stack_memory + 4096);

    // 3. Forge the stack (walking downwards)
    *(--stack) = (uint32_t)task_exit;
    *(--stack) = (uint32_t)entry_point;
    *(--stack) = 0x202; // EFLAGS (IF = 1)

    // pushad zeroes
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

    new_task->pid = allocate_pid();
    if (new_task->pid == 0)
    {
        destroy_task(new_task);
        return 0;
    }

    new_task->name = name;
    new_task->state = TASK_READY;

    new_task->stack_allocation = stack_memory;
    new_task->kernel_stack_top = 0;
    new_task->page_directory = paging_get_kernel_directory();
    new_task->wake_time = 0;

    return new_task;
}

task_t* create_user_task(const char* name, const void* code, uint32_t code_size, uint32_t user_eip, uint32_t user_esp)
{
    // BUG-010: Validate payload buffer and single-page size limit
    if (name == 0 || code == 0 || code_size == 0 || code_size > PAGE_SIZE)
        return 0;

    // Validate page alignment and user-space bounds for code and stack
    if ((user_eip & 0xFFFu) != 0 || (user_esp & 0xFFFu) != 0 || user_esp < PAGE_SIZE)
        return 0;

    uint32_t user_stack_vaddr = user_esp - PAGE_SIZE;
    if (user_eip > (USER_SPACE_END - (PAGE_SIZE - 1u)) ||
        user_stack_vaddr > (USER_SPACE_END - (PAGE_SIZE - 1u)))
        return 0;

    task_t* new_task = (task_t*)kmalloc(sizeof(task_t));
    if (new_task == 0) return 0;

    void* stack_memory = kmalloc(PAGE_SIZE);
    if (stack_memory == 0)
    {
        kfree(new_task);
        return 0;
    }

    uint32_t* page_directory = paging_create_address_space();
    if (page_directory == 0)
    {
        kfree(stack_memory);
        kfree(new_task);
        return 0;
    }

    uint32_t code_phys = (uint32_t)pmm_alloc_block();
    if (code_phys == 0)
    {
        paging_destroy_address_space(page_directory);
        kfree(stack_memory);
        kfree(new_task);
        return 0;
    }

    uint32_t stack_phys = (uint32_t)pmm_alloc_block();
    if (stack_phys == 0)
    {
        pmm_free_block((void*)code_phys);
        paging_destroy_address_space(page_directory);
        kfree(stack_memory);
        kfree(new_task);
        return 0;
    }

    // BUG-017: Copy user code and zero user stack via temporary virtual mapping
    // so physical frames >= 4 MiB never fault.
    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    void* temp_code = paging_temp_map(code_phys);
    memset(temp_code, 0, PAGE_SIZE);
    memcpy(temp_code, code, code_size);

    void* temp_stack = paging_temp_map(stack_phys);
    memset(temp_stack, 0, PAGE_SIZE);
    paging_temp_unmap();

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");

    // BUG-002, BUG-003, BUG-004: Map code and stack into the task's address space
    // and roll back all resources if either mapping fails.
    if (!paging_map_page(page_directory, user_eip, code_phys, PTE_PRESENT | PTE_USER))
    {
        pmm_free_block((void*)stack_phys);
        pmm_free_block((void*)code_phys);
        paging_destroy_address_space(page_directory);
        kfree(stack_memory);
        kfree(new_task);
        return 0;
    }

    if (!paging_map_page(page_directory, user_stack_vaddr, stack_phys, PTE_PRESENT | PTE_RW | PTE_USER))
    {
        // Note: code_phys is now mapped in page_directory and will be freed by paging_destroy_address_space()
        pmm_free_block((void*)stack_phys);
        paging_destroy_address_space(page_directory);
        kfree(stack_memory);
        kfree(new_task);
        return 0;
    }

    uint32_t* stack = (uint32_t*)((uint8_t*)stack_memory + PAGE_SIZE);

    // Save the top of the kernel stack for the TSS on Ring 3 -> Ring 0 interrupts
    new_task->kernel_stack_top = (uint32_t)stack;

    // 1. Forge arguments for enter_usermode(user_eip, user_esp)
    *(--stack) = user_esp;
    *(--stack) = user_eip;

    // 2. Fake return address for the C calling convention
    *(--stack) = (uint32_t)task_exit;

    // 3. EIP: switch_task will 'ret' into enter_usermode
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

    new_task->pid = allocate_pid();
    if (new_task->pid == 0)
    {
        destroy_task(new_task);
        return 0;
    }

    new_task->name = name;
    new_task->state = TASK_READY;

    new_task->stack_allocation = stack_memory;
    new_task->page_directory = page_directory;
    new_task->wake_time = 0;

    return new_task;
}
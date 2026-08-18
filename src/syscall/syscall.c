#include "syscall.h"
#include "../interrupts/idt.h"
#include "../task/scheduler.h"

void syscall_handler(registers_t *regs)
{
    switch (regs->eax)
    {
        case SYS_TEST:
            regs->eax = 42;
            break;

        case SYS_GETPID:
            if (current_task != 0)
                regs->eax = current_task->pid;
            else
                regs->eax = 0;
            break;

        case SYS_YIELD:
            yield();
            regs->eax = 0;
            break;

        case SYS_SLEEP:
            task_sleep(regs->ebx);
            regs->eax = 0;
            break;

        case SYS_EXIT:
            task_exit();
            break;

        default:
            regs->eax = (uint32_t)-1;
            break;
    }
}
#include "syscall.h"
#include "../interrupts/idt.h"
#include "../task/scheduler.h"
#include "../drivers/vga.h"
#include "../mm/paging.h"

static int32_t sys_write(const char* buffer, uint32_t length)
{
    if (buffer == 0) return -1;

    if (!user_range_valid((uint32_t)buffer, length))
        return -1;

    for (uint32_t i = 0; i < length; ++i)
    {
        putchar(buffer[i]);
    }
    return (int32_t)length;
}

void syscall_handler(registers_t *regs)
{
    switch (regs->eax)
    {
        case SYS_TEST:
            regs->eax = 42;
            break;

        case SYS_WRITE:
            regs->eax = (uint32_t)sys_write((const char*)regs->ebx, regs->ecx);
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
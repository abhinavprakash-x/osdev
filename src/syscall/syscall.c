#include "syscall/syscall.h"
#include "interrupts/idt.h"
#include "task/scheduler.h"
#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "drivers/fs.h"
#include "task/elf.h"
#include "mm/heap.h"
#include "mm/paging.h"

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

static int32_t sys_read(char* buffer, uint32_t length)
{
    if (buffer == 0) return -1;
    if (length == 0) return 0;

    if (!user_range_valid((uint32_t)buffer, length)) return -1;
    if (!paging_is_mapped_range((uint32_t)buffer, length, true)) return -1;

    uint32_t count = 0;
    while (count < length)
    {
        char c = keyboard_get_char();
        while (c == 0)
        {
            __asm__ volatile ("sti; hlt; cli" : : : "memory");
            yield();
            c = keyboard_get_char();
        }

        if (c == '\n')
        {
            putchar('\n');
            break;
        }
        else if (c == '\b')
        {
            if (count > 0)
            {
                putchar('\b');
                count--;
            }
            continue;
        }
        buffer[count++] = c;
        putchar(c);
    }
    return (int32_t)count;
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
            task_exit_with_status((int32_t)regs->ebx);
            break;

        case SYS_WAITPID:
        {
            int32_t status = 0;
            if (!task_wait_pid(regs->ebx, &status)) regs->eax = (uint32_t)-1;
            else
            {
                if(regs->ecx != 0)
                {
                    if (!user_range_valid(regs->ecx, sizeof(int32_t)) ||
                        !paging_is_mapped_range(regs->ecx, sizeof(int32_t), true))
                    {
                        regs->eax = (uint32_t)-1;
                        break;
                    }
                    *(int32_t*)regs->ecx = status;
                }
                regs->eax = (uint32_t)status;
            }
            break;
        }

        case SYS_READ:
            regs->eax = (uint32_t)sys_read((char*)regs->ebx, regs->ecx);
            break;

        default:
            regs->eax = (uint32_t)-1;
            break;
    }
}
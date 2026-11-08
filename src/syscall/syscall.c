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

static int32_t sys_exec(const char* user_filename)
{
    if (user_filename == 0) return -1;

    // Safely copy the null-terminated filename from User Space into Kernel Space
    char kfilename[FS_MAX_FILENAME];
    uint32_t i = 0;
    for (; i < FS_MAX_FILENAME; ++i)
    {
        if (!user_range_valid((uint32_t)&user_filename[i], 1))
            return -1;

        kfilename[i] = user_filename[i];
        if (kfilename[i] == '\0')
            break;
    }

    // Reject empty strings or strings that were not null-terminated within FS_MAX_FILENAME
    if (i == 0 || i == FS_MAX_FILENAME)
        return -1;

    const fs_entry_t* entry = fs_find(kfilename);
    if (entry == 0) return -1;

    void* elf_buffer = 0;
    uint32_t elf_size = 0;
    if (!fs_read_file(kfilename, &elf_buffer, &elf_size))
        return -1;

    task_t* task = create_user_task_elf(entry->name, elf_buffer, elf_size);
    kfree(elf_buffer);

    if (task == 0) return -1;

    uint32_t pid = task->pid;
    task_add(task);
    return (int32_t)pid;
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

        case SYS_EXEC:
            regs->eax = (uint32_t)sys_exec((const char*)regs->ebx);
            break;

        default:
            regs->eax = (uint32_t)-1;
            break;
    }
}
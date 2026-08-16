#include "syscall.h"
#include "../libc/stdlib.h"
#include "../interrupts/idt.h"

void syscall_handler(registers_t *regs)
{
    switch (regs->eax)
    {
        case SYS_TEST:
            regs->eax = 42;
            break;

        default:
            regs->eax = (uint32_t)-1;
            break;
    }
}
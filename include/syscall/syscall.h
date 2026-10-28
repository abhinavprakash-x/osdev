#pragma once

#include "interrupts/idt.h"

#define SYS_TEST    0
#define SYS_WRITE   1
#define SYS_EXIT    2
#define SYS_YIELD   3
#define SYS_SLEEP   4
#define SYS_GETPID  5
#define SYS_WAITPID 6

void syscall_handler(registers_t *regs);
#pragma once
#include "interrupts/idt.h"

#define SYS_TEST 0

void syscall_handler(registers_t *regs);
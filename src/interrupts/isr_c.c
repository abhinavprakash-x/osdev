/*
 * Interrupt Service Routine (ISR) Dispatcher
 * This file receives all CPU exceptions and hardware interrupts from the
 * assembly stubs and routes them to the correct driver or handles the crash.
 */

#include "drivers/vga.h"
#include "libc/stdlib.h"
#include "interrupts/idt.h"
#include "syscall/syscall.h"
#include "task/scheduler.h"

extern void keyboard_handler(void);
extern void pit_handler(void);

// Standard list of x86 CPU exceptions
char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved"
};

static void print_page_fault_details(uint32_t err_code)
{
    uint32_t fault_addr;
    __asm__ volatile("mov %%cr2, %0" : "=r"(fault_addr));

    printf("Faulting Address: 0x%x (", fault_addr);
    printf("%s, ", (err_code & 0x1u) ? "protection-violation" : "non-present");
    printf("%s, ", (err_code & 0x2u) ? "write" : "read");
    printf("%s)\n", (err_code & 0x4u) ? "user-mode" : "kernel-mode");
}

void isr_handler(registers_t *regs)
{
    // IRQ0: Programmable Interval Timer
    if(regs->int_no == 32)
    {
        pit_handler();
    }
    // IRQ1: PS/2 Keyboard
    else if(regs->int_no == 33)
    {
        keyboard_handler();
    }
    // Handle system calls
    else if(regs->int_no == 128)
    {
        syscall_handler(regs);
    }
    // CPU Exceptions (0-31)
    else if(regs->int_no < 32)
    {
        // If the exception occurred in Ring 3 (User Mode), terminate only the
        // faulting user task instead of panicking the kernel.
        if ((regs->cs & 0x3u) == 0x3u)
        {
            printf("\n[USER FAULT] - Task '%s' (PID %d) crashed: %s (Exception %d)\n",
                   (current_task != 0) ? current_task->name : "unknown",
                   (current_task != 0) ? (int)current_task->pid : -1,
                   exception_messages[regs->int_no],
                   regs->int_no);
            printf("Error Code: 0x%x | User EIP: 0x%x | User ESP: 0x%x\n",
                   regs->err_code, regs->eip, regs->useresp);

            if (regs->int_no == 14)
            {
                print_page_fault_details(regs->err_code);
            }

            printf("Terminating faulting user task.\n");
            task_exit();
            return;
        }

        printf("\n[KERNEL PANIC] - CPU EXCEPTION: %d\n", regs->int_no);
        printf("%s\n", exception_messages[regs->int_no]);
        printf("Error Code: 0x%x | EIP: 0x%x\n", regs->err_code, regs->eip);

        if(regs->int_no == 14)
        {
            print_page_fault_details(regs->err_code);
        }

        printf("\nSystem Halted.\n");

        while (1) {
            __asm__ volatile("cli; hlt");
        }
    }
}
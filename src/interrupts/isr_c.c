/*
 * Interrupt Service Routine (ISR) Dispatcher
 * This file receives all CPU exceptions and hardware interrupts from the 
 * assembly stubs and routes them to the correct driver or handles the crash.
 */

#include "../drivers/vga.h"
#include "../libc/stdlib.h"
#include "idt.h"

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
    // CPU Exceptions (0-31)
    else if(regs->int_no < 32)
    {
        printf("\n[KERNEL PANIC] - CPU EXCEPTION: %d\n", regs->int_no);
        printf("%s\n", exception_messages[regs->int_no]);
        
        // Print the exact instruction that caused the crash and the error code
        printf("Error Code: %d | EIP: 0x%x\n", regs->err_code, regs->eip);

        if(regs->int_no == 14)
        {
            uint32_t fault_addr;
            // The CPU stores the unmapped memory address in the CR2 register
            __asm__ volatile("mov %%cr2, %0" : "=r"(fault_addr));
            printf("Faulting Memory Address: 0x%x\n", fault_addr);
        }
        
        printf("\nSystem Halted.\n");

        // Halt the CPU completely. cli disables further interrupts, 
        // and hlt puts the CPU to sleep to save power.
        while(1) {
            __asm__ volatile("cli; hlt");
        }
    }
}
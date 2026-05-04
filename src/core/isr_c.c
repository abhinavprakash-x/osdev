#include "../drivers/vga.h"
#include "idt.h"

extern void keyboard_handler();

// Standard list of CPU exceptions
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
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

void isr_handler(registers_t *regs) {
    //if interrupt33
    if(regs->int_no == 33)
    {
        keyboard_handler();
    }
    else if(regs->int_no < 32)
    {
        print("\n[KERNEL PANIC] - CPU EXCEPTION FIRED\n");
        print(exception_messages[regs->int_no]);
        print("\nSystem Halted.\n");
        
        // An exception usually means something fatally broke.
        // Freeze the system so it doesn't do further damage.
        while(1);
    }
}
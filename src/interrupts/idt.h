#pragma once
#include <stdint.h>

// A single entry int the Interrupt Descriptor Table (IDT)
struct idt_entry {
    uint16_t offset_low;    // Lower 16 bits of the ISR's memory address
    uint16_t selector;      // Kernel segment selector
    uint8_t zero;           // Unused, always set to 0
    uint8_t type_attr;      // Flags (Present, Privilege Level, Gate Type)
    uint16_t offset_high;   // Upper 16 bits of the ISR's memory address
} __attribute__((packed));

// The descriptor pointer required by the 'lidt' assembly instruction
struct idt_ptr {
    uint16_t limit; // Size of the IDT array minus 1
    uint32_t base;  // Memory address of the first IDT entry
} __attribute__((packed));

// Represents the CPU state pushed to the stack when an interrupt occurs.
typedef struct {
    uint32_t ds;                                     // Data segment selector
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by `pushad`
    uint32_t int_no, err_code;                       // Interrupt number and error code
    uint32_t eip, cs, eflags, useresp, ss;           // Pushed by the CPU automatically
} registers_t;

void idt_init(void);
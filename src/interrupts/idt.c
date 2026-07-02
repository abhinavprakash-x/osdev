#include "idt.h"

#define IDT_ENTRIES 256

extern void idt_load(uint32_t);

// Declare all the generated assembly functions
extern void isr0(); extern void isr1(); extern void isr2(); extern void isr3();
extern void isr4(); extern void isr5(); extern void isr6(); extern void isr7();
extern void isr8(); extern void isr9(); extern void isr10(); extern void isr11();
extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15();
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19();
extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23();
extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27();
extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31();
extern void isr32(); extern void isr33();

static struct idt_entry* idt = (struct idt_entry*)0x90000;
static struct idt_ptr idtp;

static void set_idt_gate(int n, uint32_t handler) {
    idt[n].offset_low  = handler & 0xFFFF;
    idt[n].selector    = 0x08;
    idt[n].zero        = 0;
    idt[n].type_attr   = 0x8E;
    idt[n].offset_high = (handler >> 16) & 0xFFFF;
}

void idt_init() {
    idtp.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idtp.base  = (uint32_t)idt;

    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt[i].offset_low = 0; idt[i].offset_high = 0;
        idt[i].selector = 0; idt[i].type_attr = 0;
    }

    // Set all 32 exception gates
    set_idt_gate(0, (uint32_t)isr0); set_idt_gate(1, (uint32_t)isr1);
    set_idt_gate(2, (uint32_t)isr2); set_idt_gate(3, (uint32_t)isr3);
    set_idt_gate(4, (uint32_t)isr4); set_idt_gate(5, (uint32_t)isr5);
    set_idt_gate(6, (uint32_t)isr6); set_idt_gate(7, (uint32_t)isr7);
    set_idt_gate(8, (uint32_t)isr8); set_idt_gate(9, (uint32_t)isr9);
    set_idt_gate(10, (uint32_t)isr10); set_idt_gate(11, (uint32_t)isr11);
    set_idt_gate(12, (uint32_t)isr12); set_idt_gate(13, (uint32_t)isr13);
    set_idt_gate(14, (uint32_t)isr14); set_idt_gate(15, (uint32_t)isr15);
    set_idt_gate(16, (uint32_t)isr16); set_idt_gate(17, (uint32_t)isr17);
    set_idt_gate(18, (uint32_t)isr18); set_idt_gate(19, (uint32_t)isr19);
    set_idt_gate(20, (uint32_t)isr20); set_idt_gate(21, (uint32_t)isr21);
    set_idt_gate(22, (uint32_t)isr22); set_idt_gate(23, (uint32_t)isr23);
    set_idt_gate(24, (uint32_t)isr24); set_idt_gate(25, (uint32_t)isr25);
    set_idt_gate(26, (uint32_t)isr26); set_idt_gate(27, (uint32_t)isr27);
    set_idt_gate(28, (uint32_t)isr28); set_idt_gate(29, (uint32_t)isr29);
    set_idt_gate(30, (uint32_t)isr30); set_idt_gate(31, (uint32_t)isr31);
    set_idt_gate(32, (uint32_t)isr32); set_idt_gate(33, (uint32_t)isr33);

    idt_load((uint32_t)&idtp);
}
#include "idt.h"

#define IDT_ENTRIES 256

extern void idt_load(uint32_t);
extern void isr0();

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
        idt[i].offset_low = 0;
        idt[i].offset_high = 0;
        idt[i].selector = 0;
        idt[i].type_attr = 0;
    }

    set_idt_gate(0, (uint32_t)isr0);

    idt_load((uint32_t)&idtp);
}
#include "gdt.h"

static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_ptr gdt_descriptor;

extern void gdt_flush(struct gdt_ptr *descriptor);

static void gdt_set_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags)
{
    gdt[index].base_low = (base & 0xFFFF);
    gdt[index].base_middle = (base >> 16) & 0xFF;
    gdt[index].base_high = (base >> 24) & 0xFF;

    gdt[index].limit_low = (limit & 0xFFFF);
    gdt[index].granularity = ((limit >> 16) & 0x0F);

    gdt[index].granularity |= (flags & 0xF0);
    gdt[index].access = access;
}

void gdt_init(void)
{
    gdt_set_entry(0, 0, 0, 0, 0);                // Null segment
    gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Kernel code segment
    gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Kernel data segment
    gdt_set_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User code segment
    gdt_set_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User data segment

    gdt_descriptor.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gdt_descriptor.base = (uint32_t)&gdt;

    gdt_flush(&gdt_descriptor);
}
#include "tss.h"
#include "gdt.h"
#include "../libc/mem.h"

static tss_entry_t tss;

void tss_init(void)
{
    // Clear the TSS
    memset(&tss, 0, sizeof(tss_entry_t));

    // Set the Ring 0 stack segment.
    // esp0 will be updated before entering userspace.
    tss.ss0 = KERNEL_DATA_SELECTOR;
    tss.esp0 = 0;

    // Set the I/O map base to the end of the TSS
    tss.iomap_base = sizeof(tss_entry_t);

    // Add the TSS descriptor to the GDT
    gdt_set_entry(5, (uint32_t)&tss, sizeof(tss_entry_t) - 1, 0x89, 0x00);
    tss_flush();
}

void tss_set_kernel_stack(uint32_t esp0)
{
    tss.esp0 = esp0;
}
ENTRY(_start)

SECTIONS
{
    . = 0x8000;

    .text :
    {
        *kernel_entry.o(.text)   /* 🔥 FORCE FIRST */
        *(.text*)
    }

    .rodata :
    {
        *(.rodata*)
    }

    .data :
    {
        *(.data*)
    }

    .bss :
    {
        __bss_start = .;
        *(COMMON)
        *(.bss*)
        __bss_end = .;
    }

    /* BUG-009: Ensure kernel + .bss never overlap the IDT (0x90000) or boot stack (0xA0000) */
    ASSERT(__bss_end <= 0x90000, "Error: Kernel .bss overlaps IDT/stack region at 0x90000")
}
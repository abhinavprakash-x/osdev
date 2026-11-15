ENTRY(_start)

SECTIONS
{
    . = 0x00100000;

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

    /* BUG-009*/
    ASSERT(__bss_end <= 0x003FF000, "Error: Kernel .bss exceeds initial 4 MiB identity map at 0x003FF000")
}
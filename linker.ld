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
        *(COMMON)
        *(.bss*)
    }
}
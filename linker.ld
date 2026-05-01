ENTRY(_start)

SECTIONS
{
    /* Kernel is loaded at 0x8000 by bootloader */
    . = 0x8000;

    /* Code */
    .text :
    {
        *(.text*)
    }

    /* Read-only data (strings, const) */
    .rodata :
    {
        *(.rodata*)
    }

    /* Initialized data */
    .data :
    {
        *(.data*)
    }

    /* Uninitialized data */
    .bss :
    {
        *(.bss*)
        *(COMMON)
    }
}
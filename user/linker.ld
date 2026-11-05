ENTRY(_start)

PHDRS
{
    text PT_LOAD FLAGS(5); /* PF_R (4) | PF_X (1) = Read + Execute */
    data PT_LOAD FLAGS(6); /* PF_R (4) | PF_W (2) = Read + Write   */
}

SECTIONS
{
    . = 0x00400000;

    .text :
    {
        *(.text.start)
        *(.text*)
        *(.rodata*)
    } :text

    . = ALIGN(0x1000);

    .data :
    {
        *(.data*)
    } :data

    .bss :
    {
        *(COMMON)
        *(.bss*)
    } :data

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame*)
        *(.note*)
    }
}
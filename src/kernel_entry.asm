[bits 32]
global _start

_start:
    extern kmain
    call kmain

hang:
    cli
    hlt
    jmp hang
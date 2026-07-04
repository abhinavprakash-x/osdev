[bits 32]
global _start

_start:
    ; Provide a clean entry point for the linker to hand off to C
    extern kmain
    call kmain

hang:
    ; If kmain ever accidentally returns, lock up the CPU safely
    cli
    hlt
    jmp hang
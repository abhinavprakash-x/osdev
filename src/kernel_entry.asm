[bits 32]
global _start

extern __bss_start
extern __bss_end
extern kmain

_start:
    ; Provide a clean entry point for the linker to hand off to C
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi

    xor eax, eax
    rep stosb       ; Repeat while ecx != 0, [edi] = al, edi++ ecx--

    call kmain

hang:
    ; If kmain ever accidentally returns, lock up the CPU safely
    cli
    hlt
    jmp hang
global _start
[bits 16]

_start:
    cli

    ; load GDT
    lgdt [gdt_descriptor]

    ; enable protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; far jump to flush pipeline
    jmp 0x08:protected_mode_start


; -------------------------
; GDT (minimal)
; -------------------------
gdt_start:
    dq 0x0000000000000000    ; null

gdt_code:
    dq 0x00cf9a000000ffff    ; code segment

gdt_data:
    dq 0x00cf92000000ffff    ; data segment

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start


; -------------------------
; 32-bit mode starts here
; -------------------------
[bits 32]
protected_mode_start:

    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov esp, 0x90000   ; stack

    extern kmain
    call kmain

hang:
    jmp hang
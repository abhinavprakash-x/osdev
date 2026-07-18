[bits 32]

; Macro for interrupts that DO NOT push an error code
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    cli
    push 0      ; Push a dummy error code to keep stack uniform with registers_t
    push %1     ; Push the interrupt number
    jmp isr_common_stub
%endmacro

; Macro for interrupts that DO push an error code
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    cli
    ; The CPU already pushed the error code, so we don't push 0
    push %1     ; Push the interrupt number
    jmp isr_common_stub
%endmacro

; Automatically generate ISRs 0 to 31
; Note: CPU pushes error codes for 8, 10, 11, 12, 13, 14, 17, 21
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_ERRCODE   21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

; Hardware IRQs
ISR_NOERRCODE 32
ISR_NOERRCODE 33

; The C function we will call in `isr_c.c`
extern isr_handler

; This is our common handler stub. It saves the CPU state, sets up the 
; kernel environment, calls C, and then restores the exact CPU state.

isr_common_stub:
    pushad          ; Push edi, esi, ebp, esp, ebx, edx, ecx, eax
    
    mov ax, ds      ; Save the current data segment to the stack
    push eax        

    mov ax, 0x10    ; Load the Kernel Data Segment (0x10 is from our GDT)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp        ; Pass a pointer to the stack (registers_t) to the C function
    call isr_handler ; Jump into C!
    add esp, 4

    pop eax         ; Restore the original data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popad           ; Restore all general purpose registers
    add esp, 8      ; Clean up the pushed error code and pushed ISR number from the stack
    sti             ; Turn interrupts back on
    iret            ; "Interrupt Return" - pops CS, EIP, EFLAGS, and SS
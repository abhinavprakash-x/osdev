[bits 32]
global idt_load

; void idt_load(uint32_t idt_ptr);
; C calling convention passes arguments on the stack.
; [esp] contains the return address
; [esp+4] contains the first argument (pointer to our idtp struct)

idt_load:
    mov eax, [esp+4]  ; Grab the IDT pointer from the stack
    lidt [eax]        ; Load the IDT into the CPU's IDTR register
    ret               ; Return back to C code
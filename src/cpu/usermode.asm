[bits 32]
global enter_usermode

; void enter_usermode(uint32_t user_eip, uint32_t user_esp);
enter_usermode:
    cli                     ; Disable interrupts during the transition
    
    mov eax, [esp + 4]      ; Fetch user_eip from arguments
    mov edx, [esp + 8]      ; Fetch user_esp from arguments

    ; Load user data segment selector (0x23) into segment registers
    mov cx, 0x23            
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx
    
    ; -----------------------------------------
    ; Forge the stack for iretd
    ; -----------------------------------------
    push 0x23               ; SS: User Data Segment Selector
    push edx                ; ESP: User Stack Pointer
    
    pushfd                  ; Push current EFLAGS
    pop ebx                 ; Pop into EBX to modify
    or ebx, 0x200           ; Set the Interrupt Enable Flag (IF) bit
    push ebx                ; Push modified EFLAGS back
    
    push 0x1b               ; CS: User Code Segment Selector (0x1B)
    push eax                ; EIP: User Instruction Pointer
    
    iretd                   ;Welcome to Ring 3.
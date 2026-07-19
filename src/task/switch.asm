[bits 32]
global switch_task

; void switch_task(uint32_t* current_esp, uint32_t* next_esp)
switch_task:
    mov eax, [esp + 4]
    mov edx, [esp + 8]

    pushfd ; push EFLAGS on stack
    pushad ; push all registers on stack

    mov [eax], esp
    mov esp, [edx]

    popad
    popfd
    ret
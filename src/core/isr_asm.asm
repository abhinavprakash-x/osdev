[bits 32]

global isr0
extern isr0_handler

isr0:
    pushad
    call isr0_handler

    cli
    jmp $
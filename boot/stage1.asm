; Stage 1: loads stage 2 (LBA 1..STAGE2_SECTORS) to 0x7E00 and jumps to it.

[org 0x7c00]
[bits 16]

%ifndef STAGE2_SECTORS
    %define STAGE2_SECTORS 4
%endif

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti

    mov [boot_drive], dl

    mov dl, [boot_drive]
    mov ah, 0x42
    mov si, DAP
    int 0x13
    jc disk_error

    mov dl, [boot_drive]
    jmp 0x0000:0x7e00

disk_error:
    mov ax, 0x0e00 | '1'
    int 0x10
    cli
    hlt
    jmp $

boot_drive db 0

align 4
DAP:
    db 0x10
    db 0
    dw STAGE2_SECTORS
    dw 0x7e00             ; offset
    dw 0x0000             ; segment
    dd 1                  ; LBA low (right after the boot sector)
    dd 0

times 510 - ($ - $$) db 0
dw 0xaa55

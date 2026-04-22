org 0x7c00

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti

    mov [boot_drive], dl

    ; print message
    mov si, msg
    call print_string

    ; load stage2 (sector 2)
    mov ah, 0x02
    mov al, 1
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [boot_drive]

    mov bx, 0x8000
    int 0x13
    jc disk_error

    jmp 0x0000:0x8000

disk_error:
    mov si, err
    call print_string
    jmp $

; ---------- print ----------
print_string:
.next:
    mov al, [si]
    cmp al, 0
    je .done
    mov ah, 0x0e
    int 0x10
    inc si
    jmp .next
.done:
    ret

msg db "Loading OS...",0x0d,0x0a,0
err db "Disk error!",0x0d,0x0a,0

boot_drive db 0

times 510 - ($ - $$) db 0
dw 0xaa55
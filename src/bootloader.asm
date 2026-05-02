[org 0x7c00]
[bits 16] ; Explicitly declare 16-bit mode for the start

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

    ; reset disk
    mov ah, 0x00
    mov dl, [boot_drive]
    int 0x13

    ; load kernel (3 sectors)
    mov bx, 0x8000
    mov dh, 0
    mov ch, 0
    mov cl, 2
    mov dl, [boot_drive]

    mov si, 3

load_loop:
    mov ah, 0x02
    mov al, 1
    int 0x13
    jc disk_error

    add bx, 512
    inc cl

    dec si
    jnz load_loop

    ; switch to protected mode
    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp 0x08:protected_mode_start

; ----------------------------
; 16-BIT FUNCTIONS (MOVED HERE)
; ----------------------------
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

disk_error:
    mov si, err
    call print_string
    jmp $

; -------------------------
; GDT
; -------------------------
gdt_start:
    dq 0x0000000000000000

gdt_code:
    dq 0x00cf9a000000ffff

gdt_data:
    dq 0x00cf92000000ffff

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start


; -------------------------
; 32-bit mode
; -------------------------
[bits 32]
protected_mode_start:

    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax

    mov esp, 0x90000

    ; disable interrupts BEFORE kernel
    cli

    jmp 0x08:0x8000

; ----------------------------
; DATA
; ----------------------------
msg db "Loading OS...", 0x0d, 0x0a, 0
err db "Disk error!", 0x0d, 0x0a, 0

boot_drive db 0

times 510 - ($ - $$) db 0
dw 0xaa55
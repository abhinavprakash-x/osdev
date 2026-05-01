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

; ----------------------------
; print message
; ----------------------------
mov si, msg
call print_string

; ----------------------------
; reset disk (ONCE, not in loop)
; ----------------------------
mov ah, 0x00
mov dl, [boot_drive]
int 0x13

; ----------------------------
; load kernel (sector-by-sector)
; ----------------------------
mov bx, 0x8000      ; memory offset
mov es, ax          ; ES = 0
mov dh, 0           ; head
mov ch, 0           ; cylinder
mov cl, 2           ; starting sector
mov dl, [boot_drive]

mov si, 2           ; number of sectors

load_loop:
mov ah, 0x02 ; BIOS read
mov al, 1 ; read 1 sector
int 0x13
jc disk_error

add bx, 512         ; move buffer forward
inc cl              ; next sector

dec si
jnz load_loop

; ----------------------------
; jump to kernel
; ----------------------------
jmp 0x0000:0x8000

disk_error:
mov si, err
call print_string
jmp $

; ----------------------------
; print string
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

; ----------------------------
; data
; ----------------------------
msg db "Loading OS...", 0x0d, 0x0a, 0
err db "Disk error!", 0x0d, 0x0a, 0

boot_drive db 0

times 510 - ($ - $$) db 0
dw 0xaa55
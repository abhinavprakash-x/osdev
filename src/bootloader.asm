org 0x7c00

start:
    cli
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov sp, 0x7c00
    sti

    mov di, buffer

    mov si, welcome
    call print_string

main_loop:
    mov si, prompt
    call print_string

    call input
    jmp main_loop

; =========================
; INPUT HANDLER
; =========================

input:
.read:
    mov ah, 0x00
    int 0x16          ; read key → AL

    cmp al, 0x0d      ; Enter?
    je .enter

    cmp al, 0x08      ; Backspace?
    je .backspace

    ; normal character
    mov [di], al
    inc di

    mov ah, 0x0e
    int 0x10          ; echo
    jmp .read

.enter:
    mov byte [di], 0  ; null terminate

    mov si, newline
    call print_string

    call compare

    mov di, buffer    ; reset buffer pointer
    jmp .read

.backspace:
    cmp di, buffer
    je .read

    dec di

    mov ah, 0x0e
    mov al, 0x08
    int 0x10

    mov al, ' '
    int 0x10

    mov al, 0x08
    int 0x10

    jmp .read

; =========================
; STRING PRINT
; =========================

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

; =========================
; STRING COMPARE
; =========================

strcmp:
.loop:
    mov al, [si]
    mov bl, [di]

    cmp al, bl
    jne .not_equal

    cmp al, 0
    je .equal

    inc si
    inc di
    jmp .loop

.equal:
    mov al, 1
    ret

.not_equal:
    mov al, 0
    ret

; =========================
; COMMAND HANDLER
; =========================

compare:
    ; help
    mov si, buffer
    mov di, cmd_help
    call strcmp
    cmp al, 1
    je .help

    ; clear
    mov si, buffer
    mov di, cmd_clear
    call strcmp
    cmp al, 1
    je .clear

    ; echo
    mov si, buffer
    mov di, cmd_echo
    call strcmp
    cmp al, 1
    je .echo

    ; unknown
    mov si, unknown
    call print_string
    ret

.help:
    mov si, help_text
    call print_string
    ret

.clear:
    mov ah, 0x00
    mov al, 0x03
    int 0x10
    ret

.echo:
    mov si, buffer
    call print_string
    mov si, newline
    call print_string
    ret

; =========================
; DATA
; =========================

welcome db "Welcome to Test OS", 0x0d, 0x0a, 0
prompt  db "> ", 0
newline db 0x0d, 0x0a, 0

cmd_help  db "help", 0
cmd_clear db "clear", 0
cmd_echo  db "echo", 0

unknown db "Unknown command", 0x0d, 0x0a, 0

help_text db "help: show help", 0x0d, 0x0a
          db "clear: clear screen", 0x0d, 0x0a
          db "echo: echo input", 0x0d, 0x0a, 0

buffer times 64 db 0

times 510 - ($ - $$) db 0
dw 0xaa55
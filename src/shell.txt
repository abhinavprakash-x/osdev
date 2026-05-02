org 0x8000

start:
    xor ax, ax
    mov ds, ax

    mov si, welcome
    call print_string

main:
    mov si, prompt
    call print_string

    call input
    call compare

    jmp main

; ================= INPUT =================

input:
    mov di, buffer

.read:
    mov ah, 0
    int 0x16

    cmp al, 0x0d
    je .done

    cmp al, 0x08
    je .back

    mov [di], al
    inc di

    mov ah, 0x0e
    int 0x10
    jmp .read

.back:
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

.done:
    mov byte [di], 0
    mov si, newline
    call print_string
    ret

; ================= PRINT =================

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

; ================= STRCMP PREFIX =================
; checks if buffer starts with command

strcmp_prefix:
.loop:
    mov al, [di]
    mov bl, [si]

    cmp bl, 0
    je .match

    cmp al, bl
    jne .no

    inc di
    inc si
    jmp .loop

.match:
    mov al, 1
    ret

.no:
    mov al, 0
    ret

; ================= COMMANDS =================

compare:
    ; help
    mov di, buffer
    mov si, cmd_help
    call strcmp_prefix
    cmp al, 1
    je .help

    ; clear
    mov di, buffer
    mov si, cmd_clear
    call strcmp_prefix
    cmp al, 1
    je .clear

    ; echo
    mov di, buffer
    mov si, cmd_echo
    call strcmp_prefix
    cmp al, 1
    je .echo

    mov si, unknown
    call print_string
    ret

.help:
    mov si, help_text
    call print_string
    ret

.clear:
    mov ah, 0
    mov al, 3
    int 0x10
    ret

.echo:
    mov si, buffer

.skip:
    cmp byte [si], ' '
    je .print
    cmp byte [si], 0
    je .done
    inc si
    jmp .skip

.print:
    inc si
    call print_string
    mov si, newline
    call print_string
    ret

.done:
    mov si, newline
    call print_string
    ret

; ================= DATA =================

welcome db "Stage2 Shell Ready",0x0d,0x0a,0
prompt db "> ",0
newline db 0x0d,0x0a,0

cmd_help db "help",0
cmd_clear db "clear",0
cmd_echo db "echo",0
cmd_load db "load",0

help_text db "help : shows this menu",0x0d,0x0a,
          db "clear : clear screen",0x0d,0x0a,
          db "echo : syntax: echo string use: print string",0x0d,0x0a,0
unknown db "Unknown command",0x0d,0x0a,0

buffer times 128 db 0
times 512 - ($ - $$) db 0
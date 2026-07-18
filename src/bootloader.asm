[org 0x7c00]
[bits 16]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti

    mov [boot_drive], dl

    ; print loading message
    mov si, msg
    call print_string

    ; reset disk (Ah=0)
    mov ah, 0x00
    mov dl, [boot_drive]
    int 0x13

    ; ----------------------------
    ; LOAD KERNEL USING LBA
    ; ----------------------------
    mov ah, 0x42           ; BIOS Extended Read function
    mov dl, [boot_drive]   ; Drive number
    mov si, DAP            ; Point SI to our Disk Address Packet
    int 0x13
    jc disk_error          ; If carry flag is set, error!

    ; -----------------------------
    ; Enable A20 Line
    ; -----------------------------
    mov ax, 0x2401         ; BIOS function to enable A20
    int 0x15               ; Call BIOS

    ; Map the RAM using E820
    call mem_map_start

    ; switch to protected mode
    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp 0x08:protected_mode_start


; ----------------------------
; 16-BIT FUNCTIONS 
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

mem_map_start:
    mov di, 0x5004 ; map will be stored at 0x5000
    xor ebx, ebx
    xor ebp, ebp   ; ebp will count number of entries
.loop:
    mov eax, 0xe820
    mov ecx, 24
    mov edx, 0x534d4150 ; magic number for SMAP
    int 0x15            ; interrupt 0x15
    jc .done

    add di, 24 ; move pointer to next slot for next entry
    inc ebp    ; increment entry count
    cmp ebx, 0 ; if ebx = 0, list ended, exit
    jne .loop
.done:
    mov [0x5000], ebp ; save number of entries at 0x5000
    ret

; -------------------------
; GDT
; -------------------------
gdt_start:
    dq 0x0000000000000000 ; Null Descriptor
gdt_code:
    dq 0x00cf9a000000ffff ; Kernel Code Segment
gdt_data:
    dq 0x00cf92000000ffff ; Kernel Data Segment
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; -------------------------
; 32-bit mode
; -------------------------
[bits 32]
protected_mode_start:
    ; Point all data segments to the Kernel Data Segment (0x10)
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov esp, 0xa0000 ; Stack Pointer at 0xA0000
    cli
    jmp 0x08:0x8000

; ----------------------------
; DATA & VARIABLES
; ----------------------------
msg db "Loading OS via LBA...", 0x0d, 0x0a, 0
err db "Disk error!", 0x0d, 0x0a, 0
boot_drive db 0

; ----------------------------
; DISK ADDRESS PACKET (DAP)
; ----------------------------
; The BIOS requires this to be strictly formatted
align 4
DAP:
    db 0x10             ; Size of this packet (always 16 bytes)
    db 0                ; Always 0
    dw 100              ; Number of sectors to read (15KB Kernel)
    dw 0x8000           ; Memory Offset to load to
    dw 0x0000           ; Memory Segment to load to (0:0x8000)
    dq 1                ; LBA address to start reading from (Sector 1)

times 510 - ($ - $$) db 0
dw 0xaa55
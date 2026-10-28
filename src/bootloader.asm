[org 0x7c00]
[bits 16]

E820_MAX_ENTRIES equ 128
SMAP_MAGIC       equ 0x534d4150

%ifndef KERNEL_MAX_SECTORS
    %define KERNEL_MAX_SECTORS 100
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

    ; print loading message
    mov si, msg
    call print_string

    ; reset disk (AH=0)
    mov ah, 0x00
    mov dl, [boot_drive]
    int 0x13

    ; ----------------------------
    ; LOAD KERNEL USING LBA
    ; ----------------------------
    mov cx, KERNEL_MAX_SECTORS  ; Total Sectors Left

.read_loop:
    cmp cx, 0
    je .read_done

    mov ax, cx
    cmp ax, 64          ; Read 64 Sectors (32 KiB) per call
    jbe .do_read
    mov ax, 64

.do_read:
    mov [DAP_sectors], ax
    push cx             ; Save Remaining Sector Count
    push ax             ; Save Chunk Sector Count

    mov ah, 0x42        ; BIOS Extended Read function
    mov dl, [boot_drive]
    mov si, DAP         ; Point SI to our Disk Address Packet
    int 0x13
    jc disk_error       ; If Carry flag is Set Error

    pop ax              ; Restore Chunk Sector Count
    pop cx              ; Restore Remaining Sector Count

    ; Advance 64-bit LBA by Chunk Size
    movzx edx, ax
    add dword [DAP_lba], edx
    add dword [DAP_lba + 4], 0

    ; Advance destination segment by (chunk * 32) paragraphs (512 / 16 = 32)
    mov dx, ax
    shl dx, 5
    add word [DAP_segment], dx

    sub cx, ax
    jmp .read_loop

.read_done:

    ; -----------------------------
    ; Enable & Verify A20 Line
    ; -----------------------------
    call enable_a20
    jc a20_error

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
    cli
    hlt
    jmp $

a20_error:
    mov si, a20_err
    call print_string
    cli
    hlt
    jmp $

; Check if A20 is enabled by comparing 0x0000:0x0500 and 0xFFFF:0x0510.
; Returns AX = 1 if enabled, AX = 0 if disabled (wrapping).
check_a20:
    pushf
    push ds
    push es
    push di
    push si

    cli
    xor ax, ax
    mov es, ax
    mov di, 0x0500

    mov ax, 0xffff
    mov ds, ax
    mov si, 0x0510

    mov al, [es:di]
    push ax
    mov al, [ds:si]
    push ax

    mov byte [es:di], 0x00
    mov byte [ds:si], 0xff
    cmp byte [es:di], 0xff

    pop ax
    mov [ds:si], al
    pop ax
    mov [es:di], al

    mov ax, 0
    je .a20_disabled
    mov ax, 1
.a20_disabled:
    pop si
    pop di
    pop es
    pop ds
    popf
    ret

enable_a20:
    call check_a20
    cmp ax, 1
    je .a20_ok

    ; Try BIOS INT 0x15 AX=0x2401
    mov ax, 0x2401
    int 0x15
    call check_a20
    cmp ax, 1
    je .a20_ok

    ; Fallback: Fast A20 Gate (Port 0x92)
    in al, 0x92
    test al, 2
    jnz .check_final
    or al, 2
    and al, 0xfe
    out 0x92, al

.check_final:
    call check_a20
    cmp ax, 1
    je .a20_ok
    stc
    ret
.a20_ok:
    clc
    ret

mem_map_start:
    mov di, 0x5004          ; entries start at 0x5004, count at 0x5000
    xor ebx, ebx
    xor ebp, ebp            ; ebp counts valid entries
.loop:
    cmp ebp, E820_MAX_ENTRIES
    jae .done

    mov dword [es:di + 20], 1

    mov eax, 0xe820
    mov ecx, 24
    mov edx, SMAP_MAGIC
    int 0x15
    jc .done

    cmp eax, SMAP_MAGIC
    jne .done

    cmp cl, 20
    jb .skip_entry

    add di, 24
    inc ebp

.skip_entry:
    cmp ebx, 0
    jne .loop
.done:
    mov [0x5000], ebp
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
msg     db "Loading OS via LBA...", 0x0d, 0x0a, 0
err     db "Disk error!", 0x0d, 0x0a, 0
a20_err db "A20 error!", 0x0d, 0x0a, 0
boot_drive db 0

; ----------------------------
; DISK ADDRESS PACKET (DAP)
; ----------------------------
align 4
DAP:
    db 0x10               ; Size of this packet (always 16 bytes)
    db 0                  ; Always 0
DAP_sectors:
    dw 0                  ; Number of sectors in current chunk (set by loop)
DAP_offset:
    dw 0x0000             ; Memory Offset (0x0000)
DAP_segment:
    dw 0x0800             ; Memory Segment (0x0800:0x0000 = physical 0x8000)
DAP_lba:
    dd 1                  ; LBA low 32 bits (start at Sector 1)
    dd 0                  ; LBA high 32 bits

times 510 - ($ - $$) db 0
dw 0xaa55
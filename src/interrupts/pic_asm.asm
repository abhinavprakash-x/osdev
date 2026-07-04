[bits 32]
global outb
global inb

; void outb(uint16_t port, uint8_t data);
outb:
    mov eax, [esp+8] ; Argument 2 (data to send) goes into 32-bit eax (which contains 8-bit al)
    mov edx, [esp+4] ; Argument 1 (I/O port number) goes into 32-bit edx (which contains 16-bit dx)
    out dx, al       ; Send the 8-bit data in AL to the 16-bit port in DX
    ret

; uint8_t inb(uint16_t port);
inb:
    mov edx, [esp+4] ; Argument 1 (I/O port number) goes into edx/dx
    in al, dx        ; Read 8-bit data from port DX into AL (return value is expected in EAX/AL)
    ret
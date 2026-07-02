[bits 32]
global outb
global inb

outb:
    mov eax, [esp+8]
    mov edx, [esp+4]
    out dx, al
    ret

inb:
    mov edx, [esp+4]
    in al, dx
    ret
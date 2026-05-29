#include "pic_c.h"

extern void outb(uint16_t port, uint8_t data);
extern uint8_t inb(uint16_t port);

void pic_init()
{
    /*
    0x20 Master PIC Command Port
    0x21 Master PIC Data Port
    0xa0 Slave PIC Command Port
    0xa1 Slave PIC Data Port
    https://osdev.jsren.co.uk/input-output/interrupts-and-the-pic
    */
    outb(0x20, 0x11); io_wait(); outb(0xa0, 0x11); io_wait(); // ICW1
    outb(0x21, 0x20); io_wait(); outb(0xa1, 0x28); io_wait(); // ICW2
    outb(0x21, 0x04); io_wait(); outb(0xa1, 0x02); io_wait(); // ICW3
    outb(0x21, 0x01); io_wait(); outb(0xa1, 0x01); io_wait(); // ICW4
    outb(0x21, 0xfc); io_wait(); outb(0xa1, 0xff);
}

void io_wait()
{
    outb(0x80, 0);
}
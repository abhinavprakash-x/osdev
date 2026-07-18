/* 
 * Programmable Interrupt Controller (PIC) setup.
 * Remaps the legacy 8259 PIC chips so their hardware IRQs (like keyboard/timer)
 * do not collide with CPU exceptions (like page faults).
 */

#include "pic.h"
#include "../drivers/port_io.h"

void pic_init(void)
{
    /*
     * 0x20 Master PIC Command Port
     * 0x21 Master PIC Data Port
     * 0xa0 Slave PIC Command Port
     * 0xa1 Slave PIC Data Port
     * 
     * https://osdev.jsren.co.uk/input-output/interrupts-and-the-pic
     *
     * ICW1: Start initialization sequence in cascade mode
     * ICW2: Set the vector offsets
     * ICW3: Tell the PICs about each other
     * ICW4: Set mode to 8086/88 mode
     */
    outb(0x20, 0x11); io_wait(); outb(0xa0, 0x11); io_wait(); // ICW1
    outb(0x21, 0x20); io_wait(); outb(0xa1, 0x28); io_wait(); // ICW2
    outb(0x21, 0x04); io_wait(); outb(0xa1, 0x02); io_wait(); // ICW3
    outb(0x21, 0x01); io_wait(); outb(0xa1, 0x01); io_wait(); // ICW4

    // Masking: 
    // 0xFC (1111 1100) -> Unmask IRQ0 (Timer) and IRQ1 (Keyboard) on Master.
    // 0xFF (1111 1111) -> Mask all interrupts on Slave.
    outb(0x21, 0xfc); io_wait(); outb(0xa1, 0xff);
}

void io_wait(void)
{
    // Port 0x80 is used for 'checkpoints' during POST. 
    // Writing to it is safe and takes just long enough to act as a hardware delay.
    outb(0x80, 0);
}
#include "keyboard.h"

void keyboard_handler()
{
    uint8_t scancode = inb(0x60);
    putchar((char)scancode);
    outb(0x20, 0x20); // end of intterupt (otherwise future interrupts won't work)
}
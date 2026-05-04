#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "core/idt.h"
#include "core/pic_c.h"

void kmain() {
    clear_screen();
    print("Welcome to OS\n");

    print("Initializing IDT...\n");
    idt_init();
    print("Sucessfully Initialized\n");

    print("Initializing PIC...\n");
    pic_init();
    print("Sucessfully Initialized\n");

    __asm__ volatile ("sti");

    while (1);
}
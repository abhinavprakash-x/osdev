#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "drivers/pit.h"
#include "core/idt.h"
#include "core/pic_c.h"

extern void shell_init();

void kmain() {
    clear_screen();
    print("Welcome to OS\n");

    print("Initializing IDT...\n");
    idt_init();
    print("Sucessfully Initialized\n");

    print("Initializing PIC...\n");
    pic_init();
    print("Sucessfully Initialized\n");

    print("Initializing PIT...\n");
    pit_init(100);
    print("Successfully Initialized with frequency of 100 ticks/second\n");

    __asm__ volatile ("sti");

    shell_init();
    while (1);
}
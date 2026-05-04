#include "drivers/vga.h"
#include "core/idt.h"

void kmain() {
    clear_screen();
    print("Welcome to OS\n");
    print("Initializing IDT...\n");
    idt_init();
    print("Sucessfully Initialized\n");

    print("Hello\nMy\nName\nis\nAbhinav\nPrakash\nWhat\nis\nyours?\n");
    print("Now CPU will halt\nand go to an\ninfinite loop\nand stuck there\n");
    print("Good Morning\nGood Afternoon\nand\nGood Night\n");

    print("Triggering interrupt...\n");
    __asm__ volatile ("int $0");

    while (1);
}
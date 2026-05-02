#include "drivers/vga.h"
#include "core/idt.h"

void kmain() {
    clear_screen();

    print("Initializing IDT...\n");

    idt_init();

    print("Triggering interrupt...\n");

    __asm__ volatile ("int $0");

    while (1);
}
#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "drivers/pit.h"
#include "interrupts/idt.h"
#include "interrupts/pic_c.h"
#include "mm/pmm.h"

extern void shell_init();

void kmain() {
    clear_screen();
    print("Welcome to Bare Minimum OS\n");

    print("Initializing IDT...\n");
    idt_init();
    print("Sucessfully Initialized\n");

    print("Initializing PIC...\n");
    pic_init();
    print("Sucessfully Initialized\n");

    print("Initializing PIT...\n");
    pit_init(100);
    print("Successfully Initialized with frequency of 100 ticks/second\n");

    print("Initializing PMM...\n");
    pmm_init();
    print("Successfully Initialized\n");

    __asm__ volatile ("sti");

    shell_init();
    while (1);
}
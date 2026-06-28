#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "drivers/pit.h"
#include "core/idt.h"
#include "core/pic_c.h"
#include "mm/pmm.h"

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

    print("Initializing PMM...\n");
    pmm_init();
    print("Successfully Initialized\n");

    __asm__ volatile ("sti");

    void* block = pmm_alloc_block();
    print("Block Allocated\n");
    pmm_free_block(block);
    print("Block Released\n");

    shell_init();
    while (1);
}
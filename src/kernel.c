#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "drivers/pit.h"
#include "interrupts/idt.h"
#include "interrupts/pic_c.h"
#include "mm/pmm.h"
#include "mm/paging.h"

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

    print("Enabling Paging...\n");
    paging_init();
    print("Successfully Enabled\n");

    __asm__ volatile ("sti");

    // Test 1 -> Kernel Should Not Panic
    // Since we've tracked upto  4 MB so this should work
    uint32_t* mapped_memory = (uint32_t*)0x100000;
    *mapped_memory = 404;
    if (*mapped_memory == 404) {
        print("Test 1 Passes.\n");
    }

    sleep(5000);

    // Test 2 -> Kernel Should Panic
    // Since we haven't mappped upto 16 MB
    uint32_t* unmapped_memory = (uint32_t*)0x1000000;
    *unmapped_memory = 123;

    shell_init();
    while (1);
}
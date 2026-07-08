/*
 * Kernel Entry Point
 * Initializes hardware drivers, sets up interrupts, configures memory 
 * management, and hands control over to the interactive shell.
 */

#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "drivers/pit.h"
#include "interrupts/idt.h"
#include "interrupts/pic_c.h"
#include "mm/pmm.h"
#include "mm/paging.h"
#include "libc/stdlib.h"
#include "mm/heap.h"

extern void shell_init(void);

void kmain(void)
{
    clear_screen();
    printf("Welcome to Bare Minimum OS\n");

    printf("Initializing IDT...\n");
    idt_init();
    printf("Sucessfully Initialized\n");

    printf("Initializing PIC...\n");
    pic_init();
    printf("Sucessfully Initialized\n");

    printf("Initializing PIT...\n");
    pit_init(100);
    printf("Successfully Initialized with frequency of 100 Hz\n");

    printf("Initializing PMM...\n");
    pmm_init();
    printf("Successfully Initialized\n");

    printf("Enabling Paging...\n");
    paging_init();
    printf("Successfully Enabled\n");

    printf("Initializing Heap...\n");
    heap_init();
    printf("Successfully Initialized\n");

    // Enable Interrupts
    __asm__ volatile ("sti");

    // Test 1: Allocate memory
    uint32_t* test_ptr = (uint32_t*)kmalloc(sizeof(uint32_t) * 10);
    printf("Memory Allocated Successfully at virtual address: 0x%x\n", (uint32_t)test_ptr);
    printf("Physical address: 0x%x\n", get_physical_addr((uint32_t)test_ptr));

    // Test 2: Write to the memory (if it doesn't crash, paging & the heap work!)
    for(int i = 0; i < 10; i++) {
        test_ptr[i] = 0xABCD; 
    }
    printf("Successfully wrote to dynamically allocated memory!\n");

    // Test 3: Free it
    kfree(test_ptr);
    printf("Successfully freed memory!\n");

    // Launch Shell
    shell_init();

    while (1)
    {
        __asm__ volatile ("hlt");
    }
}
/*
 * Kernel Entry Point
 * Initializes hardware drivers, sets up interrupts, configures memory 
 * management, and hands control over to the interactive shell.
 */

#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "drivers/pit.h"
#include "interrupts/idt.h"
#include "interrupts/pic.h"
#include "mm/pmm.h"
#include "mm/paging.h"
#include "libc/stdlib.h"
#include "mm/heap.h"
#include "task/task.h"
#include "task/scheduler.h"

extern void shell_init(void);
extern void shell_input(char c);

uint32_t counter_a = 0;
uint32_t counter_b = 0;

void task_a(void)
{
    while(1)
    {
        printf("Counter A: %d\n", counter_a++);
        sleep(2000);
    }
}

void task_b(void)
{
    while(1)
    {
        printf("Counter B: %d\n", counter_b++);
        sleep(3000);
    }
}

void task_shell(void)
{
    while (1)
    {
        char c = keyboard_get_char();
        if (c != 0) shell_input(c);
        else __asm__ volatile ("hlt");
    }
}

void kmain(void)
{
    clear_screen();
    printf("Welcome to Bare Minimum OS\n");

    printf("Initializing IDT...\n");
    idt_init();
    printf("Successfully Initialized\n");

    printf("Initializing PIC...\n");
    pic_init();
    printf("Successfully Initialized\n");

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

    printf("Initializing Scheduler...\n");
    scheduler_init();
    printf("Successfully Initialized\n");

    // Enable Interrupts
    __asm__ volatile ("sti");

    // Test Scheduler
    task_add(create_task(task_a));
    task_add(create_task(task_b));
    task_add(create_task(task_shell));

    // Launch Shell
    shell_init();

    while (1)
    {
        __asm__ volatile ("hlt");
    }
}
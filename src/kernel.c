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
    printf("Bare Minimum OS\n");
    printf("32-bit x86 Operating System\n");
    printf("----------------------------------------\n\n");
    printf("Initializing kernel...\n");

    idt_init();
    printf("[ OK ] IDT\n");

    pic_init();
    printf("[ OK ] PIC\n");

    pit_init(100);
    printf("[ OK ] PIT                         100 Hz\n");

    pmm_init();
    printf("[ OK ] Physical Memory Manager\n");

    paging_init();
    printf("[ OK ] Paging\n");

    heap_init();
    printf("[ OK ] Kernel Heap\n");

    scheduler_init();
    printf("[ OK ] Scheduler\n");

    printf("\n----------------------------------------\n");
    printf("Kernel initialization complete.\n");

    // Launch Shell
    task_add(create_task("shell", task_shell));
    shell_init();

    // Enable Interrupts
    __asm__ volatile ("sti");

    while (1)
    {
        __asm__ volatile ("hlt");
    }
}
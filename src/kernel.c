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
#include "libc/mem.h"
#include "mm/heap.h"
#include "task/task.h"
#include "task/scheduler.h"
#include "cpu/gdt.h"
#include "cpu/tss.h"

extern void enter_usermode(uint32_t user_eip, uint32_t user_esp);
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

    gdt_init();
    printf("[ OK ] GDT\n");

    tss_init(0xA0000); // Set the initial kernel stack pointer
    printf("[ OK ] TSS\n");
    
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

    // Test User Mode (temporary, remove later)
    printf("\nTesting User Mode...\n");
    printf("Press Ctrl+Alt+2 in Qemu\nThen type info registers to verify that the CPU is in user mode.\n");
    printf("If it says\nEAX: 2a and EIP: 0040007 CPL: 3\nThen the test was successful.\n");
    extern uint8_t user_test_start[];
    extern uint8_t user_test_end[];

    uint32_t user_code_size = (uint32_t)(user_test_end - user_test_start);
    uint32_t user_code_phys = (uint32_t)pmm_alloc_block();
    memcpy((void*)user_code_phys, user_test_start, user_code_size);
    map_page(0x400000, user_code_phys, PTE_PRESENT | PTE_USER);

    uint32_t user_stack_phys = (uint32_t)pmm_alloc_block();
    map_page(0x800000, user_stack_phys, PTE_PRESENT | PTE_RW | PTE_USER);
    enter_usermode(0x400000, 0x800000 + 4096);

    task_add(create_user_task("user_test", 0x400000, 0x800000 + 4096));

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
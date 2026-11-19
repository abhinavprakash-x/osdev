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
#include "cpu/gdt.h"
#include "cpu/tss.h"
#include "drivers/fs.h"
#include "drivers/vbe.h"

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

    if(fs_init()) printf("[ OK ] Filesystem (BMFS)\n");
    else printf("[WARN] Filesystem not found\n");

    printf("\n----------------------------------------\n");
    printf("Kernel initialization complete.\n");

    // Launch Shell
    task_add(create_task("shell", task_shell));
    shell_init();

    // Enable Interrupts
    __asm__ volatile ("sti");

    vbe_init();
    put_pixel(0, 0, 0xffffffff); // Set the first pixel to white
    put_pixel(1, 0, 0xff0000ff); // Set the second pixel to red
    put_pixel(2, 0, 0xff00ff00); // Set the third pixel to green
    put_pixel(3, 0, 0xffff0000); // Set the fourth pixel to blue
    put_pixel(4, 0, 0xffffff00); // Set the fifth pixel to yellow
    put_pixel(5, 0, 0xff00ffff); // Set the sixth pixel to cyan
    put_pixel(6, 0, 0xffff00ff); // Set the seventh pixel to magenta
    put_pixel(7, 0, 0xffffffff); // Set the eighth pixel to white
    put_pixel(8, 0, 0xff808080); // Set the ninth pixel to gray
    put_pixel(9, 0, 0xffc0c0c0); // Set the tenth pixel to light gray
    put_pixel(10, 0, 0xff404040); // Set the eleventh pixel to dark gray
    put_pixel(11, 0, 0xff800000); // Set the twelfth pixel to dark red
    put_pixel(12, 0, 0xff008000); // Set the thirteenth pixel to dark green
    put_pixel(13, 0, 0xff000080); // Set the fourteenth pixel to dark blue
    put_pixel(14, 0, 0xff808000); // Set the fifteenth pixel to olive
    put_pixel(15, 0, 0xff800080); // Set the sixteenth pixel to purple
    put_pixel(16, 0, 0xff008080); // Set the seventeenth pixel to teal
    put_pixel(17, 0, 0xffffa500); // Set the eighteenth pixel to orange
    put_pixel(18, 0, 0xffa52a2a); // Set the nineteenth pixel to brown
    put_pixel(19, 0, 0xffadd8e6); // Set the twentieth pixel to light blue
    put_pixel(20, 0, 0xfff5f5dc); // Set the twenty-first pixel to beige

    while (1)
    {
        __asm__ volatile ("hlt");
    }
}
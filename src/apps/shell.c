/*
 * Shell Application
 * Provides an interactive command-line interface for the user.
 * Note: Currently executes in Ring 0 (Kernel Mode). In future
 * will migrate this to Ring 3 (User Mode) with system calls.
 */

#include "../drivers/vga.h"
#include "../libc/string.h"
#include "../libc/mem.h"
#include "../libc/stdlib.h"
#include "../drivers/pit.h"
#include "kernel_tests.h"
#include "../mm/pmm.h"
#include "../mm/heap.h"
#include "../task/scheduler.h"

static char buffer[256];
static int buffer_index = 0;

void shell_init(void)
{
    memset(buffer, 0, sizeof(buffer));
    buffer_index = 0;

    printf("\nWelcome to Bare Minimum OS!\n");
    printf("Type 'help' for available commands.\n\n");
    printf("OS> ");
}

void execute_command(void)
{
    if(strlen(buffer) == 0)
    {
        printf("OS> ");
        return;
    }
    if (strcmp(buffer, "clear") == 0)
    {
        clear_screen();
    }
    else if (strcmp(buffer, "help") == 0)
    {
        printf("\nAvailable commands:\n");

        printf("  General\n");
        printf("    help                 Show available commands\n");
        printf("    clear                Clear the screen\n");
        printf("    echo <text>          Print text to the console\n");
        printf("    calc <expression>    Evaluate a basic expression\n");
        printf("    time                 Show time since boot\n");
        printf("    reboot               Restart the system\n");

        printf("\n  System\n");
        printf("    cpuinfo              Display CPU information\n");
        printf("    meminfo              Display physical memory usage\n");
        printf("    tasks                Display active tasks\n");
        printf("    test [suite]         Run kernel tests\n");

        printf("\n  Memory\n");
        printf("    alloc <bytes>        Allocate kernel heap memory\n");
        printf("    free <address>       Free a heap allocation\n");
        printf("    peek <address>       Read a 32-bit value from memory\n");
        printf("    poke <addr> <value>  Write a 32-bit value to memory\n");

        printf("\n  Debug\n");
        printf("    crash div0           Trigger a divide-by-zero exception\n");
        printf("    crash page           Trigger a page fault\n");
    }  
    else if (strncmp(buffer, "echo ", 5) == 0)
    {
        // print everything after the "echo " command
        printf("%s\n",buffer + 5);
    }
    else if (strncmp(buffer, "calc ", 5) == 0)
    {
        char* eqn_l = buffer + 5; // pointer to the first operand
        char operator = 0;
        char* eqn_r = 0; // pointer to the second operand
        int i = 1;

        // Parse the equation to find the operator and split the string
        while(eqn_l[i] != '\0')
        {
            if (eqn_l[i] == '+' || eqn_l[i] == '-' || eqn_l[i] == '*' || eqn_l[i] == '/')
            {
                operator = eqn_l[i];
                eqn_l[i] = '\0';
                eqn_r = &eqn_l[i + 1];
                break;
            }
            i++;
        }
        if(operator != 0)
        {
            int num1 = atoi(eqn_l);
            int num2 = atoi(eqn_r);
            int result = 0;
            int error = 0;

            if (operator == '+') result = num1 + num2;
            else if (operator == '-') result = num1 - num2;
            else if (operator == '*') result = num1 * num2;
            else if (operator == '/')
            {
                if (num2 == 0)
                {
                    printf("Error: Division by zero\n");
                    error = 1;
                }
                else result = num1 / num2;
            }
            if(!error) printf("%d\n", result);
        }
        else printf("Error: Invalid syntax. Use format: calc 5+3\n");
    }
    else if (strcmp(buffer, "time") == 0)
    {
        // PIT runs at 100Hz, so ticks / 100 = seconds
        printf("Seconds since boot: %d\n", get_ticks() / 100);
    }
    else if (strncmp(buffer, "test", 4) == 0)
    {
        if(strcmp(buffer, "test") == 0 || strcmp(buffer, "test all") == 0) test_all();
        else if(strcmp(buffer, "test string") == 0) test_string();
        else if(strcmp(buffer, "test stdlib") == 0) test_stdlib();
        else if(strcmp(buffer, "test memory") == 0) test_memory();
        else if(strcmp(buffer, "test scheduler") == 0) test_scheduler();
        else printf("Unknown test suite.\n");
    }
    else if (strcmp(buffer, "meminfo") == 0)
    {
        // Convert 4096-byte blocks into Kilobytes (1 Block = 4 KB)
        int used_kb = get_used_memory() * 4;
        int total_kb = get_total_memory() * 4;
        int free_kb = total_kb - used_kb;
        
        printf("--- Physical Memory ---\n");
        printf("Total : %d KB\n", total_kb);
        printf("Used  : %d KB\n", used_kb);
        printf("Free  : %d KB\n", free_kb);
    }
    else if (strncmp(buffer, "alloc ", 6) == 0)
    {
        char* buf = buffer + 6;
        int* ptr = (int*)kmalloc(atoi(buf));
        char buf2[32];
        itox((int)ptr, buf2);
        printf("%d Bytes Allocated at Address 0x%s \n", atoi(buf), buf2);
    }
    else if(strncmp(buffer, "free ", 5) == 0)
    {
        char* buf = buffer + 5;
        int addr = xtoi(buf);
        kfree((void*)addr);
        printf("Memory Freed Successfully.\n");
    }
    else if (strncmp(buffer, "peek ", 5) == 0)
    {
        char* arg = buffer + 5;
        uint32_t addr = (uint32_t)xtoi(arg);
        uint32_t* ptr = (uint32_t*)addr;
        
        char val_str[32];
        itox(*ptr, val_str);
        printf("Value at 0x%s is: 0x%s\n", arg, val_str);
    }
    else if (strncmp(buffer, "poke ", 5) == 0)
    {
        char* arg = buffer + 5;
        int i = 0;
        
        // Find the space separating the address and the value
        while(arg[i] != ' ' && arg[i] != '\0') 
        {
            i++;
        }
        
        if (arg[i] == ' ') 
        {
            arg[i] = '\0';
            char* val_arg = &arg[i + 1];
            
            uint32_t addr = (uint32_t)xtoi(arg);
            uint32_t val = (uint32_t)xtoi(val_arg);
            
            uint32_t* ptr = (uint32_t*)addr;
            *ptr = val;
            
            printf("Successfully poked memory.\n");
        } 
        else 
        {
            printf("Syntax: poke <address> <value>\n");
        }
    }
    else if (strcmp(buffer, "cpuinfo") == 0)
    {
        uint32_t ebx, edx, ecx;
        
        // Fire the CPUID instruction
        __asm__ volatile (
            "cpuid"
            : "=b"(ebx), "=d"(edx), "=c"(ecx)
            : "a"(0)
        );
        
        char vendor[13];
        *(uint32_t*)&vendor[0] = ebx;
        *(uint32_t*)&vendor[4] = edx;
        *(uint32_t*)&vendor[8] = ecx;
        vendor[12] = '\0'; // Null terminate
        
        printf("CPU Vendor: %s\n", vendor);
    }
    else if (strncmp(buffer, "crash ", 6) == 0)
    {
        char* arg = buffer + 6;
        if (strcmp(arg, "div0") == 0) 
        {
            volatile int a = 1; 
            volatile int b = 0; 
            a = a / b; // Triggers Exception 0
        } 
        else if (strcmp(arg, "page") == 0) 
        {
            volatile uint32_t* bad_ptr = (uint32_t*)0x80000000;
            *bad_ptr = 0xBAD; // Triggers Exception 14
        } 
        else 
        {
            printf("Syntax: crash <div0|page>\n");
        }
    }
    else if (strcmp(buffer, "reboot") == 0)
    {
        printf("Rebooting system...\n");
        // Send command 0xFE to the Keyboard Controller port 0x64
        __asm__ volatile ("outb %1, %0" : : "dN" ((uint16_t)0x64), "a" ((uint8_t)0xFE));
    }
    else if (strcmp(buffer, "tasks") == 0)
    {
        print_tasks();
    }
    else
    {
        printf("Unknown command: %s\n", buffer);
    }

    // Clear the buffer for next command
    memset(buffer, 0, 256);
    buffer_index = 0;
    printf("OS> ");
}

void shell_input(char c)
{
    if (c == '\n')
    {
        printf("\n");
        buffer[buffer_index] = '\0';
        execute_command();
    }
    else if (c == '\b')
    {
        if (buffer_index > 0)
        {
            buffer_index--;
            buffer[buffer_index] = '\0';
            putchar('\b');
        }
    }
    else
    {
        if (buffer_index < 255)
        {
            buffer[buffer_index] = c;
            buffer_index++;
            putchar(c);
        }
    }
}

void shell_clear_screen(void)
{
    clear_screen();
    memset(buffer, 0, 256);
    buffer_index = 0;
    printf("OS> ");
}
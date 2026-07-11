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

static char buffer[256];
static int buffer_index = 0;

void shell_init(void) 
{
    memset(buffer, 0, 256); 
    printf("Welcome to the OS Shell!\n");
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
        printf("Available commands:\n");
        printf("help: Shows this menu\n");
        printf("clear: Clears the screen (F1)\n");
        printf("echo <string>: Prints string on console\n");
        printf("calc <eqn>: Calculator\n");
        printf("time: Shows Seconds since last boot\n");
        printf("test: Runs the Test Suite\n");
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
        int i = 0;

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
        else printf("Unknown test suite.\n");
    }
    else if (strcmp(buffer, "meminfo") == 0)
    {
        printf("Used Memory: %d Bytes\n", get_used_memory() * 4096);
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
            int a = 1; 
            volatile int b = 0; 
            a = a / b; // Triggers Exception 0
        } 
        else if (strcmp(arg, "page") == 0) 
        {
            volatile uint32_t* bad_ptr = (uint32_t*)0xFFFFFFFF;
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
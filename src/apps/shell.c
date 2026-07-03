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

static char buffer[256];
static int buffer_index = 0;

void shell_init() 
{
    memset(buffer, 0, 256); 
    printf("Welcome to the OS Shell!\n");
    printf("OS> ");
}

void execute_command()
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
                    print("Error: Division by zero\n");
                    error = 1;
                }
                else result = num1 / num2;
            }
            if(!error) printf("%d\n", result);
        }
        else print("Error: Invalid syntax. Use format: calc 5+3\n");
    }
    else if (strcmp(buffer, "time") == 0)
    {
        // PIT runs at 100Hz, so ticks / 100 = seconds
        printf("Seconds since boot: %d\n", get_ticks() / 100);
    }
    else
    {
        printf("Unknown command: %s\n", buffer);
    }

    // Clear the buffer for next command
    memset(buffer, 0, 256);
    buffer_index = 0;
    print("OS> ");
}

void shell_input(char c)
{
    if (c == '\n')
    {
        print("\n");
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

void shell_clear_screen()
{
    clear_screen();
    memset(buffer, 0, 256);
    buffer_index = 0;
    printf("OS> ");
}
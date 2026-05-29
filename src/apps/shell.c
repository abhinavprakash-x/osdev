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
    print("Welcome to the OS Shell!\n");
    print("OS> ");
}

void execute_command()
{
    if(strlen(buffer) == 0)
    {
        print("OS> ");
        return;
    }
    if (strcmp(buffer, "clear") == 0)
    {
        clear_screen();
    }
    else if (strcmp(buffer, "help") == 0)
    {
        print("Available commands:\n");
        print("help: Shows this menu\n");
        print("clear: Clears the screen (F1)\n");
        print("echo <string>: Prints string on console\n");
        print("calc <eqn>: Calculator\n");
        print("time: Shows Seconds since last boot\n");
    }  
    else if (strncmp(buffer, "echo ", 5) == 0)
    {
        print(buffer + 5);
        print("\n");
    }
    else if (strncmp(buffer, "calc ", 5) == 0)
    {
        char* eqn_l = buffer + 5;
        char operator = 0;
        char* eqn_r = 0;
        int i = 0;

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

            if (operator == '+') result = num1 + num2;
            else if (operator == '-') result = num1 - num2;
            else if (operator == '*') result = num1 * num2;
            else if (operator == '/')
            {
                if (num2 == 0)
                {
                    print("Error: Division by zero\n");
                    return;
                }
                result = num1 / num2;
            }
            char result_str[32];
            itoa(result, result_str);
            print(result_str);
            print("\n");
        }
        else print("Error: Invalid syntax. Use format: calc 5+3\n");
    }
    else if (strcmp(buffer, "time") == 0)
    {
        char buf[32];
        itoa(get_ticks() / 100, buf); // Divide by 100 to get seconds!
        print("Seconds since boot: ");
        print(buf);
        print("\n");
    }
    else
    {
        print("Unknown command: ");
        print(buffer);
        print("\n");
    }
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
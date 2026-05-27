#include "../drivers/vga.h"
#include "../libc/string.h"
#include "../libc/mem.h"

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
    if(strlen(buffer) == 0) print("OS> ");
    if (strcmp(buffer, "clear") == 0) clear_screen();
    else if (strcmp(buffer, "help") == 0)
    {
        print("Available commands:\n");
        print("help: Shows this menu\n");
        print("clear: Clears the screen (F1)\n");
        print("echo <string>: Prints string on console\n");
    }  
    else if (strncmp(buffer, "echo ", 5) == 0)
    {
        print(buffer + 5);
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
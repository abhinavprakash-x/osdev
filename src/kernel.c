#include "drivers/vga.h"

void kmain() {

    clear_screen();
    const char *msg = "Hello from C kernel!";
    print(msg);
    print("Hello World!");
    print("\nAbhinav\nPrakash\nOSDev");

    while (1);
}
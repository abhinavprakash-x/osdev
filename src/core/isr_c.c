#include "../drivers/vga.h"

void isr0_handler() {
    print("Divide by zero exception!\n");
}
#include "pit.h"

static volatile uint32_t tick_count = 0;

void pit_handler()
{
    tick_count++;
}

void pit_init(uint32_t freq)
{
    uint32_t divisor = 1193180 / freq; // Hardware Clock frequency is 1193180
    outb(0x43, 0x36);

    uint8_t low = (uint8_t)(divisor & 0xFF);
    uint8_t high = (uint8_t)((divisor >> 8) & 0xFF);
    outb(0x40, low);
    outb(0x40, high);
}

uint32_t get_ticks()
{
    return tick_count;
}

void sleep(uint32_t milliseconds)
{
    // If our PIT runs at 100Hz, 1 tick = 10 milliseconds.
    // So if the user wants to sleep for 500ms, they need to wait 50 ticks.
    uint32_t target_ticks = tick_count + (milliseconds / 10);
    
    // Halt the CPU until the target tick is reached
    while (tick_count < target_ticks) {
        // 'hlt' puts the CPU to sleep until the next interrupt fires.
        // This stops your OS from burning 100% CPU power on a while loop!
        __asm__ volatile("hlt"); 
    }
}
/*
 * Programmable Interval Timer (PIT) Driver
 * Configures the hardware timer (8253/8254 chip) to fire interrupts on IRQ0.
 * Used for tracking system uptime and providing sleep/delay functionality.
 */

#include "pit.h"

// Hardware I/O Ports
#define PIT_CMD_PORT      0x43
#define PIT_CHANNEL0_PORT 0x40
#define PIC1_COMMAND      0x20
#define PIC_EOI           0x20
// Internal Clock Frequency of the PIT chip (1.193182 MHz)
#define PIT_BASE_FREQ     1193180

extern void outb(uint16_t port, uint8_t data);

static volatile uint32_t tick_count = 0;
static uint32_t timer_freq = 0;

void pit_handler(void)
{
    tick_count++;
    outb(PIC1_COMMAND, PIC_EOI);
}

void pit_init(uint32_t freq)
{
    if (freq < 20) freq = 20;
    
    timer_freq = freq;
    uint32_t divisor = PIT_BASE_FREQ / freq;

    // Command 0x36: Channel 0, Lobyte/Hibyte, Square Wave Mode, 16-bit binary
    outb(PIT_CMD_PORT, 0x36);

    uint8_t low = (uint8_t)(divisor & 0xFF);
    uint8_t high = (uint8_t)((divisor >> 8) & 0xFF);

    outb(PIT_CHANNEL0_PORT, low);
    outb(PIT_CHANNEL0_PORT, high);
}

uint32_t get_ticks(void)
{
    return tick_count;
}

uint32_t ticks_to_ms(uint32_t ticks)
{
    if (timer_freq == 0)
        return 0;

    return (ticks * 1000) / timer_freq;
}

void sleep(uint32_t milliseconds)
{
    if (timer_freq == 0) return;

    // Calculate ticks needed based on the actual configured frequency
    // Formula: (ms * frequency) / 1000
    uint32_t target_ticks = tick_count + ((milliseconds * timer_freq) / 1000);
    
    // Halt the CPU until the target tick is reached hlt puts CPU into a
    // Low power state until any interrupt fires.
    while (tick_count < target_ticks) {
        __asm__ volatile("hlt");
    }
}
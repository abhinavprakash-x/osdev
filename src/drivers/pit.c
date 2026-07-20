/*
 * Programmable Interval Timer (PIT) Driver
 * Configures the hardware timer (8253/8254 chip) to fire interrupts on IRQ0.
 * Used for tracking system uptime and providing sleep/delay functionality.
 */

#include "pit.h"
#include "port_io.h"
#include "../task/scheduler.h"

// Hardware I/O Ports
#define PIT_CMD_PORT      0x43
#define PIT_CHANNEL0_PORT 0x40
#define PIC1_COMMAND      0x20
#define PIC_EOI           0x20
// Internal Clock Frequency of the PIT chip (1.193182 MHz)
#define PIT_BASE_FREQ     1193180

static volatile uint32_t tick_count = 0;
static uint32_t timer_freq = 0;
static int time_slice = 0;

void pit_handler(void)
{
    tick_count++;
    outb(PIC1_COMMAND, PIC_EOI);

    time_slice++;
    if (time_slice >= 5)
    {
        time_slice = 0;
        schedule();
    }
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

uint32_t get_timer_freq(void)
{
    return timer_freq;
}
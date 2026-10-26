#pragma once
#include <stdint.h>
#include <stdbool.h>

#define MAX_SLEEP_TICKS 0x7FFFFFFFu

void pit_init(uint32_t freq);
void pit_handler(void);

uint32_t get_ticks(void);
uint32_t get_timer_freq(void);

uint32_t ticks_to_ms(uint32_t ticks);
uint32_t ms_to_ticks(uint32_t milliseconds);

static inline bool ticks_reached(uint32_t now, uint32_t wake_time)
{
    return (int32_t)(now - wake_time) >= 0;
}
#pragma once
#include <stdint.h>

void pit_init(uint32_t freq);
void pit_handler(void);
uint32_t get_ticks(void);
uint32_t get_timer_freq(void);
uint32_t ticks_to_ms(uint32_t ticks);
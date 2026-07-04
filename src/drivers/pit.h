#pragma once
#include <stdint.h>

void pit_init(uint32_t freq);
void pit_handler(void);
void sleep(uint32_t milliseconds);
uint32_t get_ticks(void);
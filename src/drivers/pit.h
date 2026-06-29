#pragma once
#include <stdint.h>
#include "../interrupts/pic_c.h"

void pit_init(uint32_t freq);
void pit_handler();
void sleep(uint32_t milliseconds);
uint32_t get_ticks();
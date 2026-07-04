#pragma once
#include <stdint.h>
#include <stdbool.h>

extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t data);

void keyboard_handler(void);
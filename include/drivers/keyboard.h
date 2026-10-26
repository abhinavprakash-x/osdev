#pragma once
#include <stdint.h>
#include <stdbool.h>

#define KEYBOARD_BUFFER_SIZE 256

void keyboard_handler(void);
char keyboard_get_char(void);

void keyboard_process_scancode(uint8_t scancode);
void keyboard_reset_state(void);
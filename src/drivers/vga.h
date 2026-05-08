#pragma once
#include <stddef.h>
#include <stdint.h>
#include "../libc/mem.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

void clear_screen();
void putchar(char c);
void print(const char* c);
void update_cursor();
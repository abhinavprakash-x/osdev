#pragma once
#include <stddef.h>
#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

void clear_screen();
void update_cursor();

void putchar(char c);
void print(const char* c);
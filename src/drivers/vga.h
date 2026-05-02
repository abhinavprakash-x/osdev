#pragma once
#include <stddef.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

void clear_screen();
void putchar(char c);
void print(const char* c);
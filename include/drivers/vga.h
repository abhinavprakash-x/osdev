#pragma once
#include <stddef.h>
#include <stdint.h>

#define VGA_WIDTH            80
#define VGA_HEIGHT           25
#define VGA_SCROLLBACK_LINES 200

void clear_screen(void);
void update_cursor(void);

void putchar(char c);
void print(const char* c);

void vga_scroll_up(size_t lines);
void vga_scroll_down(size_t lines);
#pragma once
#include <stddef.h>
#include <stdint.h>

void console_init(void);
void console_set_colors(uint32_t fg, uint32_t bg);
void console_redraw(void);

void clear_screen(void);
void putchar(char c);
void print(const char* s);

void console_scroll_up(size_t lines);
void console_scroll_down(size_t lines);
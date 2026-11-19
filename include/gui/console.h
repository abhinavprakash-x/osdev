#pragma once
#include <stddef.h>
#include <stdint.h>
#include "gui/surface.h"

typedef void (*console_dirty_cb_t)(uint32_t x, uint32_t y, uint32_t w, uint32_t h);

void console_attach(surface_t* surface, console_dirty_cb_t on_dirty);
void console_set_colors(uint32_t fg, uint32_t bg);
void console_redraw(void);

void clear_screen(void);
void putchar(char c);
void print(const char* s);

void console_scroll_up(size_t lines);
void console_scroll_down(size_t lines);
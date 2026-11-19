#pragma once
#include <stdint.h>
#include <stdbool.h>

#define FONT_WIDTH  8u
#define FONT_HEIGHT 16u

uint32_t font_get_width(void);
uint32_t font_get_height(void);

void font_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg, uint32_t bg, bool draw_bg);
void font_draw_string(uint32_t x, uint32_t y, const char* s, uint32_t fg, uint32_t bg, bool draw_bg);
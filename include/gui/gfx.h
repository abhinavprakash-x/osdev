#pragma once
#include <stdint.h>
#include "gui/surface.h"

void gfx_put_pixel(surface_t* dst, uint32_t x, uint32_t y, uint32_t color);
void gfx_clear(surface_t* dst, uint32_t color);
void gfx_fill_rect(surface_t* dst, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void gfx_draw_rect(surface_t* dst, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void gfx_draw_line(surface_t* dst, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);

void gfx_scroll_rect_up(surface_t* dst, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                        uint32_t dy, uint32_t fill_color);

void gfx_blit_surface(surface_t* dst, int32_t dst_x, int32_t dst_y, const surface_t* src);
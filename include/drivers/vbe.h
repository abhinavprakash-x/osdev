#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t attributes;
    uint8_t  window_a, window_b;
    uint16_t granularity;
    uint16_t window_size;
    uint16_t segment_a, segment_b;
    uint32_t win_func_ptr;
    uint16_t pitch;         // Offset 16
    uint16_t width;         // Offset 18
    uint16_t height;        // Offset 20
    uint8_t  w_char, y_char, planes, bpp, banks; // bpp is at Offset 25
    uint8_t  memory_model, bank_size, image_pages, reserved0;
    uint8_t  red_mask, red_position;
    uint8_t  green_mask, green_position;
    uint8_t  blue_mask, blue_position;
    uint8_t  reserved_mask, reserved_position;
    uint8_t  direct_color_attributes;
    uint32_t phys_base;     // Offset 40
    uint32_t offscreen_mem_offset;
    uint16_t offscreen_mem_size;
    uint8_t  reserved1[206];
} __attribute__((packed)) vbe_mode_info_t;

bool vbe_init(void);
bool vbe_is_initialized(void);

uint32_t vbe_get_width(void);
uint32_t vbe_get_height(void);
uint32_t vbe_get_pitch(void);

uint32_t vbe_rgb(uint8_t r, uint8_t g, uint8_t b);

void vbe_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void vbe_clear(uint32_t color);
void vbe_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void vbe_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void vbe_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);

void vbe_blit_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void vbe_swap(void);
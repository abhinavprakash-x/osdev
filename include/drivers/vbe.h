#pragma once
#include <stdint.h>

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

void vbe_init(void);
void put_pixel(uint32_t x, uint32_t y, uint32_t color);
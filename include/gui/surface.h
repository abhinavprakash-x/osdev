#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t* pixels;   // Backing 32-bit pixel buffer
    uint32_t  width;    // Width in pixels
    uint32_t  height;   // Height in pixels
    uint32_t  pitch;    // Bytes per scanline
} surface_t;

bool surface_alloc(surface_t* out, uint32_t width, uint32_t height);
void surface_free(surface_t* surface);
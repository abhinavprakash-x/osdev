#include "gui/gfx.h"
#include "mm/heap.h"
#include "libc/mem.h"

static inline uint32_t* surface_row_ptr(const surface_t* s, uint32_t y)
{
    return (uint32_t*)((uint8_t*)s->pixels + (y * s->pitch));
}

static inline void fill_u32(uint32_t* dst, uint32_t val, uint32_t count)
{
    __asm__ volatile (
        "cld\n\t"
        "rep stosl"
        : "+D"(dst), "+c"(count)
        : "a"(val)
        : "memory"
    );
}

bool surface_alloc(surface_t* out, uint32_t width, uint32_t height)
{
    if (out == 0 || width == 0 || height == 0) return false;

    uint32_t pitch = width * 4u;
    if (pitch / 4u != width) return false;
    uint32_t total = pitch * height;
    if (total / height != pitch) return false;

    uint32_t* buf = (uint32_t*)kmalloc(total);
    if (buf == 0) return false;

    memset(buf, 0, total);
    out->pixels = buf;
    out->width  = width;
    out->height = height;
    out->pitch  = pitch;
    return true;
}

void surface_free(surface_t* surface)
{
    if (surface == 0) return;
    if (surface->pixels != 0)
    {
        kfree(surface->pixels);
        surface->pixels = 0;
    }
    surface->width  = 0;
    surface->height = 0;
    surface->pitch  = 0;
}

void gfx_put_pixel(surface_t* dst, uint32_t x, uint32_t y, uint32_t color)
{
    if (dst == 0 || dst->pixels == 0) return;
    if (x >= dst->width || y >= dst->height) return;

    surface_row_ptr(dst, y)[x] = color;
}

void gfx_clear(surface_t* dst, uint32_t color)
{
    if (dst == 0 || dst->pixels == 0) return;
    gfx_fill_rect(dst, 0, 0, dst->width, dst->height, color);
}

void gfx_fill_rect(surface_t* dst, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    if (dst == 0 || dst->pixels == 0 || w == 0 || h == 0) return;
    if (x >= dst->width || y >= dst->height) return;

    if (w > dst->width - x)  w = dst->width - x;
    if (h > dst->height - y) h = dst->height - y;

    if (x == 0 && w == dst->width && dst->pitch == dst->width * 4u)
    {
        fill_u32(surface_row_ptr(dst, y), color, w * h);
        return;
    }

    for (uint32_t dy = 0; dy < h; ++dy)
    {
        fill_u32(surface_row_ptr(dst, y + dy) + x, color, w);
    }
}

void gfx_draw_rect(surface_t* dst, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    if (dst == 0 || dst->pixels == 0 || w == 0 || h == 0) return;

    gfx_fill_rect(dst, x, y, w, 1u, color);
    if (h > 1u) gfx_fill_rect(dst, x, y + h - 1u, w, 1u, color);
    if (h > 2u)
    {
        gfx_fill_rect(dst, x, y + 1u, 1u, h - 2u, color);
        if (w > 1u) gfx_fill_rect(dst, x + w - 1u, y + 1u, 1u, h - 2u, color);
    }
}

void gfx_draw_line(surface_t* dst, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color)
{
    if (dst == 0 || dst->pixels == 0) return;

    int32_t dx = (x1 >= x0) ? (x1 - x0) : (x0 - x1);
    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t dy = (y1 >= y0) ? -(y1 - y0) : -(y0 - y1);
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t err = dx + dy;

    while (1)
    {
        if (x0 >= 0 && y0 >= 0)
            gfx_put_pixel(dst, (uint32_t)x0, (uint32_t)y0, color);

        if (x0 == x1 && y0 == y1) break;
        int32_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void gfx_scroll_rect_up(surface_t* dst, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                        uint32_t dy, uint32_t fill_color)
{
    if (dst == 0 || dst->pixels == 0 || w == 0 || h == 0 || dy == 0) return;
    if (x >= dst->width || y >= dst->height) return;

    if (w > dst->width - x)  w = dst->width - x;
    if (h > dst->height - y) h = dst->height - y;

    if (dy >= h)
    {
        gfx_fill_rect(dst, x, y, w, h, fill_color);
        return;
    }

    uint32_t copy_rows = h - dy;
    uint32_t row_bytes = w * 4u;

    if (x == 0 && w == dst->width && dst->pitch == row_bytes)
    {
        // Forward copy from higher address to lower address is overlap-safe
        memcpy(surface_row_ptr(dst, y),
               surface_row_ptr(dst, y + dy),
               copy_rows * dst->pitch);
    }
    else
    {
        for (uint32_t row_i = 0; row_i < copy_rows; ++row_i)
        {
            memcpy(surface_row_ptr(dst, y + row_i) + x,
                   surface_row_ptr(dst, y + row_i + dy) + x,
                   row_bytes);
        }
    }

    gfx_fill_rect(dst, x, y + copy_rows, w, dy, fill_color);
}

void gfx_blit_surface(surface_t* dst, int32_t dst_x, int32_t dst_y, const surface_t* src)
{
    if (dst == 0 || dst->pixels == 0 || src == 0 || src->pixels == 0) return;
    if (src->width == 0 || src->height == 0) return;

    int32_t sx = 0;
    int32_t sy = 0;
    int32_t copy_w = (int32_t)src->width;
    int32_t copy_h = (int32_t)src->height;

    if (dst_x < 0) { sx -= dst_x; copy_w += dst_x; dst_x = 0; }
    if (dst_y < 0) { sy -= dst_y; copy_h += dst_y; dst_y = 0; }

    if (dst_x >= (int32_t)dst->width || dst_y >= (int32_t)dst->height) return;

    if (dst_x + copy_w > (int32_t)dst->width)  copy_w = (int32_t)dst->width  - dst_x;
    if (dst_y + copy_h > (int32_t)dst->height) copy_h = (int32_t)dst->height - dst_y;
    if (copy_w <= 0 || copy_h <= 0) return;

    uint32_t row_bytes = (uint32_t)copy_w * 4u;
    for (int32_t row_i = 0; row_i < copy_h; ++row_i)
    {
        memcpy(surface_row_ptr(dst, (uint32_t)(dst_y + row_i)) + (uint32_t)dst_x,
               surface_row_ptr(src, (uint32_t)(sy + row_i)) + (uint32_t)sx,
               row_bytes);
    }
}
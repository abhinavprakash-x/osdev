#include "drivers/vbe.h"
#include "mm/paging.h"
#include "mm/heap.h"
#include "libc/mem.h"

#define VBE_INFO_ADDR    0x6000u
#define VBE_VIRTUAL_BASE 0xE0000000u

static uint8_t* lfb = 0;
static uint8_t* back_buffer = 0;
static bool     has_back_buffer = false;
static bool     vbe_ready = false;

static uint32_t fb_width  = 0;
static uint32_t fb_height = 0;
static uint32_t fb_pitch  = 0;
static uint32_t fb_size   = 0;

static uint8_t  red_shift   = 16;
static uint8_t  green_shift = 8;
static uint8_t  blue_shift  = 0;

bool vbe_init(void)
{
    const vbe_mode_info_t* info = (const vbe_mode_info_t*)VBE_INFO_ADDR;

    if (info->width == 0 || info->height == 0 || info->bpp != 32 || info->phys_base == 0)
        return false;

    fb_width  = info->width;
    fb_height = info->height;
    fb_pitch  = info->pitch;
    fb_size   = fb_height * fb_pitch;

    if (fb_pitch < fb_width * 4u || fb_size == 0)
        return false;

    if (info->red_mask != 0 || info->green_mask != 0 || info->blue_mask != 0)
    {
        red_shift   = info->red_position;
        green_shift = info->green_position;
        blue_shift  = info->blue_position;
    }

    uint32_t phys_base   = info->phys_base & 0xFFFFF000u;
    uint32_t phys_offset = info->phys_base & 0xFFFu;
    uint32_t pages       = (fb_size + phys_offset + PAGE_SIZE - 1u) / PAGE_SIZE;

    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    for (uint32_t i = 0; i < pages; ++i)
    {
        uint32_t phys = phys_base + (i * PAGE_SIZE);
        uint32_t virt = VBE_VIRTUAL_BASE + (i * PAGE_SIZE);
        if (!map_page_mmio(virt, phys, PTE_PRESENT | PTE_RW))
        {
            for (uint32_t j = 0; j < i; ++j)
                unmap_page(VBE_VIRTUAL_BASE + (j * PAGE_SIZE));
            __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
            return false;
        }
    }

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");

    lfb = (uint8_t*)(VBE_VIRTUAL_BASE + phys_offset);

    back_buffer = (uint8_t*)kmalloc(fb_size);
    if (back_buffer != 0)
    {
        has_back_buffer = true;
        memset(back_buffer, 0, fb_size);
    }
    else
    {
        // Fallback to direct LFB writes if RAM back buffer allocation fails
        back_buffer = lfb;
        has_back_buffer = false;
    }

    vbe_ready = true;
    return true;
}

bool vbe_is_initialized(void) { return vbe_ready; }
uint32_t vbe_get_width(void)  { return fb_width; }
uint32_t vbe_get_height(void) { return fb_height; }
uint32_t vbe_get_pitch(void)  { return fb_pitch; }

uint32_t vbe_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)r << red_shift) |
           ((uint32_t)g << green_shift) |
           ((uint32_t)b << blue_shift);
}

void vbe_put_pixel(uint32_t x, uint32_t y, uint32_t color)
{
    if (!vbe_ready || x >= fb_width || y >= fb_height) return;
    uint32_t* pixel = (uint32_t*)(back_buffer + (y * fb_pitch) + (x * 4u));
    *pixel = color;
}

void vbe_clear(uint32_t color)
{
    if (!vbe_ready) return;
    vbe_fill_rect(0, 0, fb_width, fb_height, color);
}

void vbe_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    if (!vbe_ready || w == 0 || h == 0) return;
    if (x >= fb_width || y >= fb_height) return;

    if (w > fb_width - x)  w = fb_width - x;
    if (h > fb_height - y) h = fb_height - y;

    for (uint32_t dy = 0; dy < h; ++dy)
    {
        uint32_t* row = (uint32_t*)(back_buffer + ((y + dy) * fb_pitch)) + x;
        for (uint32_t dx = 0; dx < w; ++dx)
        {
            row[dx] = color;
        }
    }
}

void vbe_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    if (!vbe_ready || w == 0 || h == 0) return;
    vbe_fill_rect(x, y, w, 1, color);
    if (h > 1) vbe_fill_rect(x, y + h - 1u, w, 1, color);
    if (h > 2)
    {
        vbe_fill_rect(x, y + 1u, 1, h - 2u, color);
        if (w > 1) vbe_fill_rect(x + w - 1u, y + 1u, 1, h - 2u, color);
    }
}

void vbe_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color)
{
    if (!vbe_ready) return;

    int32_t dx = (x1 >= x0) ? (x1 - x0) : (x0 - x1);
    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t dy = (y1 >= y0) ? -(y1 - y0) : -(y0 - y1);
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t err = dx + dy;

    while (1)
    {
        if (x0 >= 0 && y0 >= 0)
            vbe_put_pixel((uint32_t)x0, (uint32_t)y0, color);

        if (x0 == x1 && y0 == y1) break;
        int32_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void vbe_blit_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    if (!vbe_ready || !has_back_buffer || w == 0 || h == 0) return;
    if (x >= fb_width || y >= fb_height) return;

    if (w > fb_width - x)  w = fb_width - x;
    if (h > fb_height - y) h = fb_height - y;

    uint32_t row_bytes = w * 4u;
    for (uint32_t dy = 0; dy < h; ++dy)
    {
        uint32_t offset = ((y + dy) * fb_pitch) + (x * 4u);
        memcpy(lfb + offset, back_buffer + offset, row_bytes);
    }
}

void vbe_swap(void)
{
    if (!vbe_ready || !has_back_buffer) return;
    memcpy(lfb, back_buffer, fb_size);
}
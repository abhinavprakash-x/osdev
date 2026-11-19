#include "drivers/vbe.h"
#include "mm/paging.h"
#include "mm/heap.h"
#include "libc/mem.h"

#define VBE_INFO_ADDR    0x6000u
#define VBE_VIRTUAL_BASE 0xE0000000u

static uint8_t*  lfb             = 0;
static surface_t back_surface    = { 0, 0, 0, 0 };
static bool      has_back_buffer = false;
static bool      vbe_ready       = false;
static uint32_t  fb_size         = 0;

static uint8_t   red_shift       = 16;
static uint8_t   green_shift     = 8;
static uint8_t   blue_shift      = 0;

bool vbe_init(void)
{
    const vbe_mode_info_t* info = (const vbe_mode_info_t*)VBE_INFO_ADDR;

    if (info->width == 0 || info->height == 0 || info->bpp != 32 || info->phys_base == 0)
        return false;

    uint32_t width  = info->width;
    uint32_t height = info->height;
    uint32_t pitch  = info->pitch;
    uint32_t size   = height * pitch;

    if (pitch < width * 4u || size == 0)
        return false;

    if (info->red_mask != 0 || info->green_mask != 0 || info->blue_mask != 0)
    {
        red_shift   = info->red_position;
        green_shift = info->green_position;
        blue_shift  = info->blue_position;
    }

    uint32_t phys_base   = info->phys_base & 0xFFFFF000u;
    uint32_t phys_offset = info->phys_base & 0xFFFu;
    uint32_t pages       = (size + phys_offset + PAGE_SIZE - 1u) / PAGE_SIZE;

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

    lfb     = (uint8_t*)(VBE_VIRTUAL_BASE + phys_offset);
    fb_size = size;

    uint8_t* ram_buf = (uint8_t*)kmalloc(fb_size);
    if (ram_buf != 0)
    {
        has_back_buffer = true;
        memset(ram_buf, 0, fb_size);
        back_surface.pixels = (uint32_t*)ram_buf;
    }
    else
    {
        has_back_buffer = false;
        back_surface.pixels = (uint32_t*)lfb;
    }

    back_surface.width  = width;
    back_surface.height = height;
    back_surface.pitch  = pitch;
    vbe_ready = true;
    return true;
}

bool       vbe_is_initialized(void) { return vbe_ready; }
uint32_t   vbe_get_width(void)      { return back_surface.width; }
uint32_t   vbe_get_height(void)     { return back_surface.height; }
uint32_t   vbe_get_pitch(void)      { return back_surface.pitch; }
surface_t* vbe_get_surface(void)    { return vbe_ready ? &back_surface : 0; }

uint32_t vbe_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)r << red_shift) |
           ((uint32_t)g << green_shift) |
           ((uint32_t)b << blue_shift);
}

void vbe_blit_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    if (!vbe_ready || !has_back_buffer || w == 0 || h == 0) return;
    if (x >= back_surface.width || y >= back_surface.height) return;

    if (w > back_surface.width - x)  w = back_surface.width - x;
    if (h > back_surface.height - y) h = back_surface.height - y;

    const uint8_t* src_base = (const uint8_t*)back_surface.pixels;

    // Fast path: full-width blit in one contiguous transfer
    if (x == 0 && w == back_surface.width)
    {
        uint32_t offset = y * back_surface.pitch;
        memcpy(lfb + offset, src_base + offset, h * back_surface.pitch);
        return;
    }

    uint32_t row_bytes = w * 4u;
    for (uint32_t dy = 0; dy < h; ++dy)
    {
        uint32_t offset = ((y + dy) * back_surface.pitch) + (x * 4u);
        memcpy(lfb + offset, src_base + offset, row_bytes);
    }
}

void vbe_swap(void)
{
    if (!vbe_ready || !has_back_buffer) return;
    memcpy(lfb, back_surface.pixels, fb_size);
}
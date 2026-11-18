#include "drivers/vbe.h"
#include "mm/paging.h"

#define VBE_INFO_ADDR 0x6000
#define VBE_VIRTUAL_BASE 0xE0000000u 

static vbe_mode_info_t* vbe_info = (vbe_mode_info_t*)VBE_INFO_ADDR;
static uint8_t* back_buffer;

void vbe_init(void)
{
    // Calculate total bytes needed for the screen
    uint32_t fb_size = vbe_info->height * vbe_info->pitch;
    uint32_t pages = (fb_size + PAGE_SIZE - 1) / PAGE_SIZE;

    // Map the hardware framebuffer into kernel virtual space
    for (uint32_t i = 0; i < pages; i++)
    {
        uint32_t phys = vbe_info->phys_base + (i * PAGE_SIZE);
        uint32_t virt = VBE_VIRTUAL_BASE + (i * PAGE_SIZE);
        map_page_mmio(virt, phys, PTE_PRESENT | PTE_RW);
    }
    back_buffer = (uint8_t*)VBE_VIRTUAL_BASE;
}

void put_pixel(int x, int y, uint32_t color)
{
    if (x < 0 || x >= vbe_info->width || y < 0 || y >= vbe_info->height) return;

    
    // 32-bit mode uses exactly 4 bytes per pixel
    uint32_t offset = (y * vbe_info->pitch) + (x * 4);
    *(uint32_t*)(back_buffer + offset) = color;
}
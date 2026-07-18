/*
 * VGA Text Mode Driver
 * Manages the 80x25 hardware text buffer at physical address 0xB8000.
 * Handles hardware cursor updates, screen scrolling, and control characters.
 */

#include "vga.h"
#include "../libc/mem.h"

// Hardware I/O Ports for the VGA CRT Controller
#define VGA_CTRL_REGISTER 0x3D4
#define VGA_DATA_REGISTER 0x3D5
#define VGA_OFFSET_LOW    0x0F
#define VGA_OFFSET_HIGH   0x0E
// Memory-mapped I/O address for VGA text mode
#define VGA_MEM_ADDR      0xB8000

extern void outb(uint16_t port, uint8_t data);
extern uint8_t inb(uint16_t port);

static volatile char* const video = (volatile char*)VGA_MEM_ADDR;
static size_t row = 0;
static size_t col = 0;
// Default Color: Light Green (0xA) on Black Background (0x0)
static const char color = 0x0A;

void clear_screen(void)
{
    // A VGA character takes 2 bytes: [ASCII Char] [Color Attribute]
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; ++i)
    {
        video[i * 2] = ' ';
        video[i * 2 + 1] = color;
    }
    row = 0;
    col = 0;
    update_cursor();
}

void putchar(char c)
{
    if (c == '\n')
    {
        col = 0;
        row++;
    }
    else if (c == '\t')
    {
        col = (col + 4) & ~3; // Align to the next multiple of 4 using bitwise math
        if (col >= VGA_WIDTH)
        {
            col = 0;
            row++;
        }
    }
    else if (c == '\b')
    {
        if (col == 0) 
        {
            if (row > 0) 
            {
                row--;
                col = VGA_WIDTH - 1;
            }
        } 
        else col--;
        size_t index = (row * VGA_WIDTH + col) * 2;
        video[index] = ' ';
        video[index + 1] = color;
    }
    else
    {
        size_t index = (row * VGA_WIDTH + col) * 2;
        video[index] = c;
        video[index + 1] = color;

        col++;
        if (col >= VGA_WIDTH)
        {
            col = 0;
            row++;
        }
    }

    // Handle screen scrolling if we hit the bottom of the screen
    if (row >= VGA_HEIGHT)
    {
        // Destination: The very start of video memory (Row 0)
        // Source: The start of Row 1 (video + VGA_WIDTH * 2 bytes)
        // Count: The total bytes in 24 rows
        size_t bytes_to_copy = VGA_WIDTH * (VGA_HEIGHT - 1) * 2;
        memcpy((void*)video, (const void*)(video + VGA_WIDTH * 2), bytes_to_copy);

        size_t last_row_start = (VGA_HEIGHT - 1) * VGA_WIDTH * 2;
        for (size_t i = 0; i < VGA_WIDTH; i++)
        {
            video[last_row_start + (i * 2)] = ' ';
            video[last_row_start + (i * 2) + 1] = color;
        }
        row = VGA_HEIGHT - 1;
    }
    update_cursor();
}

void print(const char* s)
{
    for (size_t i = 0; s[i] != '\0'; ++i)
    {
        putchar(s[i]);
    }
}

void update_cursor(void)
{
    size_t index = row * VGA_WIDTH + col;
    uint8_t high = (index >> 8);
    uint8_t low = index & 0xff;

    outb(VGA_CTRL_REGISTER, VGA_OFFSET_LOW); 
    outb(VGA_DATA_REGISTER, low);

    outb(VGA_CTRL_REGISTER, VGA_OFFSET_HIGH); 
    outb(VGA_DATA_REGISTER, high);
}
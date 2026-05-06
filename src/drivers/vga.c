#include "vga.h"

extern void outb(uint16_t port, uint8_t data);
extern uint8_t inb(uint16_t port);

static volatile char* const video = (volatile char*)0xB8000;
static size_t row = 0;
static size_t col = 0;
static const char color = 0x07;

void clear_screen(void)
{
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; ++i)
    {
        video[i * 2]     = ' ';
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
        col = (col + 4) & ~3;
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

    if (row >= VGA_HEIGHT)
    {
        for(size_t i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1) * 2; ++i)
        {
            video[i] = video[i + VGA_WIDTH * 2];
        }
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

void update_cursor()
{
    size_t index = row * VGA_WIDTH + col;
    uint8_t high = (index >> 8);
    uint8_t low = index & 0xff;
    outb(0x3d4, 0x0f); outb(0x3d5, low);
    outb(0x3d4, 0x0e); outb(0x3d5, high);
}
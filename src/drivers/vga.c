#include "vga.h"

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
}

void putchar(char c)
{
    if (c == '\n')
    {
        col = 0;
        row++;
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
}

void print(const char* s)
{
    for (size_t i = 0; s[i] != '\0'; ++i)
    {
        putchar(s[i]);
    }
}
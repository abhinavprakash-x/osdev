/*
 * VGA Text Mode Driver
 * Manages the 80x25 hardware text buffer at physical address 0xB8000.
 * Handles hardware cursor updates, screen scrolling, and control characters.
 */

#include "drivers/vga.h"
#include "libc/mem.h"
#include "drivers/port_io.h"

// Hardware I/O Ports for the VGA CRT Controller
#define VGA_CTRL_REGISTER 0x3D4
#define VGA_DATA_REGISTER 0x3D5
#define VGA_OFFSET_LOW    0x0F
#define VGA_OFFSET_HIGH   0x0E

// Memory-mapped I/O address for VGA text mode
#define VGA_MEM_ADDR      0xB8000

static volatile char* const video = (volatile char*)VGA_MEM_ADDR;
static char back_buffer[VGA_WIDTH * VGA_SCROLLBACK_LINES * 2];

static size_t row = 0;
static size_t col = 0;
static size_t scroll_offset = 0;

// Default Color: Light Green (0xA) on Black Background (0x0)
static const char color = 0x0A;

static size_t get_bottom_view_start(void)
{
    return (row >= VGA_HEIGHT) ? (row - (VGA_HEIGHT - 1)) : 0;
}

static void flush_buffer(void)
{
    size_t bottom_start = get_bottom_view_start();
    if(scroll_offset > bottom_start) scroll_offset = bottom_start;

    size_t view_start = bottom_start - scroll_offset;
    const char* src = back_buffer + (view_start * VGA_WIDTH * 2);

    memcpy((void*)video, src, VGA_WIDTH * VGA_HEIGHT * 2);
}

void vga_scroll_up(size_t lines)
{
    size_t max_offset = get_bottom_view_start();
    if(scroll_offset + lines > max_offset) scroll_offset = max_offset;
    else scroll_offset += lines;

    flush_buffer();
    update_cursor();
}

void vga_scroll_down(size_t lines)
{
    if(scroll_offset < lines) scroll_offset = 0;
    else scroll_offset -= lines;

    flush_buffer();
    update_cursor();
}

void clear_screen(void)
{
    // A VGA character takes 2 bytes: [ASCII Char] [Color Attribute]
    for(size_t i = 0; i < VGA_WIDTH * VGA_SCROLLBACK_LINES; ++i)
    {
        back_buffer[i * 2] = ' ';
        back_buffer[i * 2 + 1] = color;
    }
    
    row = 0;
    col = 0;
    scroll_offset = 0;

    flush_buffer();
    update_cursor();
}

void putchar(char c)
{
    scroll_offset = 0; // Reset scroll offset to show the latest output

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
        back_buffer[index] = ' ';
        back_buffer[index + 1] = color;
    }
    else
    {
        size_t index = (row * VGA_WIDTH + col) * 2;
        back_buffer[index] = c;
        back_buffer[index + 1] = color;

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
        size_t row_size = VGA_WIDTH * 2;
        size_t bytes_to_copy = VGA_WIDTH * (VGA_SCROLLBACK_LINES - 1) * 2;

        memcpy(back_buffer, back_buffer + row_size, bytes_to_copy);

        size_t last_row_start = (VGA_SCROLLBACK_LINES - 1) * VGA_WIDTH * 2;
        for (size_t i = 0; i < VGA_WIDTH; i++)
        {
            back_buffer[last_row_start + (i * 2)] = ' ';
            back_buffer[last_row_start + (i * 2) + 1] = color;
        }
        row = VGA_HEIGHT - 1;
        col = 0;
    }
    flush_buffer();
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
    size_t index;

    if (scroll_offset > 0)
    {
        // Hide the hardware cursor off-screen while viewing scrollback history
        index = VGA_WIDTH * VGA_HEIGHT;
    }
    else
    {
        size_t screen_row = (row >= VGA_HEIGHT) ? (VGA_HEIGHT - 1) : row;
        index = screen_row * VGA_WIDTH + col;
    }
    
    uint8_t high = (index >> 8);
    uint8_t low = index & 0xff;

    outb(VGA_CTRL_REGISTER, VGA_OFFSET_LOW); 
    outb(VGA_DATA_REGISTER, low);

    outb(VGA_CTRL_REGISTER, VGA_OFFSET_HIGH); 
    outb(VGA_DATA_REGISTER, high);
}
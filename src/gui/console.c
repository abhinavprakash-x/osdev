#include "gui/console.h"
#include "gui/font.h"
#include "drivers/vbe.h"
#include "libc/mem.h"
#include <stdbool.h>

#define CONSOLE_MAX_COLS         128u
#define CONSOLE_SCROLLBACK_LINES 200u

static char     text_buffer[CONSOLE_SCROLLBACK_LINES][CONSOLE_MAX_COLS];
static uint32_t cols          = 128u;
static uint32_t visible_rows  = 48u;
static uint32_t cell_w        = 8u;
static uint32_t cell_h        = 16u;

static size_t   cur_row       = 0;
static size_t   cur_col       = 0;
static size_t   scroll_offset = 0;

static uint32_t fg_color      = 0x0055FF55u;
static uint32_t bg_color      = 0x000C0F14u;
static bool     console_ready = false;

static size_t get_bottom_view_start(void)
{
    return (cur_row >= visible_rows) ? (cur_row - (visible_rows - 1u)) : 0;
}

static void draw_cell(size_t screen_row, size_t screen_col, char ch, bool with_cursor)
{
    if (!console_ready || screen_row >= visible_rows || screen_col >= cols)
        return;

    uint32_t px = (uint32_t)screen_col * cell_w;
    uint32_t py = (uint32_t)screen_row * cell_h;

    font_draw_char(px, py, ch, fg_color, bg_color, true);

    if (with_cursor && cell_h >= 2u)
    {
        vbe_fill_rect(px, py + cell_h - 2u, cell_w, 2u, fg_color);
    }
}

static void redraw_all_unlocked(void)
{
    if (!console_ready) return;

    size_t bottom_start = get_bottom_view_start();
    if (scroll_offset > bottom_start) scroll_offset = bottom_start;
    size_t view_start = bottom_start - scroll_offset;

    vbe_clear(bg_color);

    for (size_t sr = 0; sr < visible_rows; ++sr)
    {
        size_t buf_row = view_start + sr;
        if (buf_row >= CONSOLE_SCROLLBACK_LINES) break;

        for (size_t sc = 0; sc < cols; ++sc)
        {
            bool is_cursor = (scroll_offset == 0 && buf_row == cur_row && sc == cur_col);
            char ch = text_buffer[buf_row][sc];
            if (ch != ' ' || is_cursor)
            {
                draw_cell(sr, sc, ch, is_cursor);
            }
        }
    }

    vbe_swap();
}

void console_init(void)
{
    if (!vbe_is_initialized()) return;

    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    cell_w = font_get_width();
    cell_h = font_get_height();

    cols = vbe_get_width() / cell_w;
    if (cols == 0) cols = 1u;
    if (cols > CONSOLE_MAX_COLS) cols = CONSOLE_MAX_COLS;

    visible_rows = vbe_get_height() / cell_h;
    if (visible_rows == 0) visible_rows = 1u;
    if (visible_rows > CONSOLE_SCROLLBACK_LINES) visible_rows = CONSOLE_SCROLLBACK_LINES;

    fg_color = vbe_rgb(0x55, 0xFF, 0x55);
    bg_color = vbe_rgb(0x0C, 0x0F, 0x14);

    console_ready = true;
    redraw_all_unlocked();

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
}

void console_set_colors(uint32_t fg, uint32_t bg)
{
    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    fg_color = fg;
    bg_color = bg;
    if (console_ready) redraw_all_unlocked();

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
}

void console_redraw(void)
{
    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    redraw_all_unlocked();

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
}

/* --- Compatibility implementation of include/drivers/vga.h --- */

void clear_screen(void)
{
    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    memset(text_buffer, ' ', sizeof(text_buffer));
    cur_row = 0;
    cur_col = 0;
    scroll_offset = 0;

    if (console_ready) redraw_all_unlocked();

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
}

void console_scroll_up(size_t lines)
{
    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    size_t max_offset = get_bottom_view_start();
    if (scroll_offset + lines > max_offset) scroll_offset = max_offset;
    else scroll_offset += lines;

    if (console_ready) redraw_all_unlocked();

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
}

void console_scroll_down(size_t lines)
{
    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    if (scroll_offset < lines) scroll_offset = 0;
    else scroll_offset -= lines;

    if (console_ready) redraw_all_unlocked();

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
}

void putchar(char c)
{
    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    bool need_full_redraw = (scroll_offset != 0);
    scroll_offset = 0;

    size_t old_row = cur_row;
    size_t old_col = cur_col;
    size_t old_screen_row = (old_row >= visible_rows) ? (visible_rows - 1u) : old_row;

    // Erase previous software cursor before moving
    if (console_ready && !need_full_redraw)
    {
        draw_cell(old_screen_row, old_col, text_buffer[old_row][old_col], false);
        vbe_blit_rect((uint32_t)old_col * cell_w, (uint32_t)old_screen_row * cell_h, cell_w, cell_h);
    }

    if (c == '\n')
    {
        cur_col = 0;
        cur_row++;
        if (cur_row >= visible_rows) need_full_redraw = true;
    }
    else if (c == '\t')
    {
        cur_col = (cur_col + 4u) & ~3u;
        if (cur_col >= cols)
        {
            cur_col = 0;
            cur_row++;
            if (cur_row >= visible_rows) need_full_redraw = true;
        }
    }
    else if (c == '\b')
    {
        if (cur_col == 0)
        {
            if (cur_row > 0)
            {
                cur_row--;
                cur_col = cols - 1u;
                if (old_row >= visible_rows) need_full_redraw = true;
            }
        }
        else
        {
            cur_col--;
        }

        text_buffer[cur_row][cur_col] = ' ';
    }
    else
    {
        text_buffer[cur_row][cur_col] = c;

        if (console_ready && !need_full_redraw)
        {
            draw_cell(old_screen_row, cur_col, c, false);
            vbe_blit_rect((uint32_t)cur_col * cell_w, (uint32_t)old_screen_row * cell_h, cell_w, cell_h);
        }

        cur_col++;
        if (cur_col >= cols)
        {
            cur_col = 0;
            cur_row++;
            if (cur_row >= visible_rows) need_full_redraw = true;
        }
    }

    // Shift scrollback history when all 200 lines are full
    if (cur_row >= CONSOLE_SCROLLBACK_LINES)
    {
        memcpy(text_buffer[0], text_buffer[1],
               (CONSOLE_SCROLLBACK_LINES - 1u) * CONSOLE_MAX_COLS);
        memset(text_buffer[CONSOLE_SCROLLBACK_LINES - 1u], ' ', CONSOLE_MAX_COLS);
        cur_row = CONSOLE_SCROLLBACK_LINES - 1u;
        cur_col = 0;
        need_full_redraw = true;
    }

    if (console_ready)
    {
        if (need_full_redraw)
        {
            redraw_all_unlocked();
        }
        else
        {
            size_t new_screen_row = (cur_row >= visible_rows) ? (visible_rows - 1u) : cur_row;
            draw_cell(new_screen_row, cur_col, text_buffer[cur_row][cur_col], true);
            vbe_blit_rect((uint32_t)cur_col * cell_w, (uint32_t)new_screen_row * cell_h, cell_w, cell_h);
        }
    }

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
}

void print(const char* s)
{
    if (s == 0) return;
    for (size_t i = 0; s[i] != '\0'; ++i)
    {
        putchar(s[i]);
    }
}
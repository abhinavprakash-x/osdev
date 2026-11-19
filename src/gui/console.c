#include "gui/console.h"
#include "gui/gfx.h"
#include "gui/font.h"
#include "libc/mem.h"
#include <stdbool.h>

#define CONSOLE_MAX_COLS         128u
#define CONSOLE_SCROLLBACK_LINES 200u

static char               text_buffer[CONSOLE_SCROLLBACK_LINES][CONSOLE_MAX_COLS];
static surface_t*         target_surface = 0;
static console_dirty_cb_t dirty_cb       = 0;

static uint32_t cols          = 128u;
static uint32_t visible_rows  = 48u;
static uint32_t cell_w        = FONT_WIDTH;
static uint32_t cell_h        = FONT_HEIGHT;

static size_t   cur_row       = 0;
static size_t   cur_col       = 0;
static size_t   scroll_offset = 0;

static uint32_t fg_color      = 0x0055FF55u;
static uint32_t bg_color      = 0x000C0F14u;

// Dirty-rect accumulator so print() emits one blit instead of one per character
static bool     dirty_valid   = false;
static uint32_t dirty_x0      = 0;
static uint32_t dirty_y0      = 0;
static uint32_t dirty_x1      = 0;
static uint32_t dirty_y1      = 0;

static inline size_t get_bottom_view_start(void)
{
    return (cur_row >= visible_rows) ? (cur_row - (visible_rows - 1u)) : 0;
}

static inline char sanitize_cell(char ch)
{
    return (ch == '\0') ? ' ' : ch;
}

static void mark_dirty_unlocked(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    if (target_surface == 0 || w == 0 || h == 0) return;

    uint32_t x1 = x + w;
    uint32_t y1 = y + h;
    if (x1 > target_surface->width)  x1 = target_surface->width;
    if (y1 > target_surface->height) y1 = target_surface->height;
    if (x >= x1 || y >= y1) return;

    if (!dirty_valid)
    {
        dirty_x0 = x;
        dirty_y0 = y;
        dirty_x1 = x1;
        dirty_y1 = y1;
        dirty_valid = true;
    }
    else
    {
        if (x  < dirty_x0) dirty_x0 = x;
        if (y  < dirty_y0) dirty_y0 = y;
        if (x1 > dirty_x1) dirty_x1 = x1;
        if (y1 > dirty_y1) dirty_y1 = y1;
    }
}

static void flush_dirty_unlocked(void)
{
    if (!dirty_valid) return;
    if (dirty_cb != 0)
    {
        dirty_cb(dirty_x0, dirty_y0, dirty_x1 - dirty_x0, dirty_y1 - dirty_y0);
    }
    dirty_valid = false;
}

static void draw_cell_unlocked(size_t screen_row, size_t screen_col, char ch, bool with_cursor)
{
    if (target_surface == 0 || screen_row >= visible_rows || screen_col >= cols)
        return;

    uint32_t px = (uint32_t)screen_col * cell_w;
    uint32_t py = (uint32_t)screen_row * cell_h;

    font_draw_char(target_surface, px, py, sanitize_cell(ch), fg_color, bg_color, true);

    if (with_cursor && cell_h >= 2u)
    {
        gfx_fill_rect(target_surface, px, py + cell_h - 2u, cell_w, 2u, fg_color);
    }
}

static void redraw_all_unlocked(void)
{
    if (target_surface == 0) return;

    size_t bottom_start = get_bottom_view_start();
    if (scroll_offset > bottom_start) scroll_offset = bottom_start;
    size_t view_start = bottom_start - scroll_offset;

    gfx_clear(target_surface, bg_color);

    for (size_t sr = 0; sr < visible_rows; ++sr)
    {
        size_t buf_row = view_start + sr;
        if (buf_row >= CONSOLE_SCROLLBACK_LINES) break;

        for (size_t sc = 0; sc < cols; ++sc)
        {
            bool is_cursor = (scroll_offset == 0 && buf_row == cur_row && sc == cur_col);
            char ch = sanitize_cell(text_buffer[buf_row][sc]);
            if (ch != ' ' || is_cursor)
            {
                draw_cell_unlocked(sr, sc, ch, is_cursor);
            }
        }
    }

    mark_dirty_unlocked(0, 0, target_surface->width, target_surface->height);
}

static void putchar_unlocked(char c)
{
    bool need_full_redraw = (scroll_offset != 0);
    bool scrolled_one     = false;
    scroll_offset = 0;

    size_t old_row        = cur_row;
    size_t old_col        = cur_col;
    size_t old_screen_row = (old_row >= visible_rows) ? (visible_rows - 1u) : old_row;

    if (c == '\n')
    {
        if (target_surface != 0 && !need_full_redraw)
        {
            draw_cell_unlocked(old_screen_row, old_col, text_buffer[old_row][old_col], false);
            mark_dirty_unlocked((uint32_t)old_col * cell_w, (uint32_t)old_screen_row * cell_h, cell_w, cell_h);
        }
        cur_col = 0;
        cur_row++;
        if (cur_row >= visible_rows && !need_full_redraw) scrolled_one = true;
    }
    else if (c == '\t')
    {
        if (target_surface != 0 && !need_full_redraw)
        {
            draw_cell_unlocked(old_screen_row, old_col, text_buffer[old_row][old_col], false);
            mark_dirty_unlocked((uint32_t)old_col * cell_w, (uint32_t)old_screen_row * cell_h, cell_w, cell_h);
        }
        cur_col = (cur_col + 4u) & ~3u;
        if (cur_col >= cols)
        {
            cur_col = 0;
            cur_row++;
            if (cur_row >= visible_rows && !need_full_redraw) scrolled_one = true;
        }
    }
    else if (c == '\b')
    {
        if (target_surface != 0 && !need_full_redraw)
        {
            draw_cell_unlocked(old_screen_row, old_col, text_buffer[old_row][old_col], false);
            mark_dirty_unlocked((uint32_t)old_col * cell_w, (uint32_t)old_screen_row * cell_h, cell_w, cell_h);
        }

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

        // Paint glyph BEFORE any scroll shift so the wrapped row keeps this character
        if (target_surface != 0 && !need_full_redraw)
        {
            draw_cell_unlocked(old_screen_row, cur_col, c, false);
            mark_dirty_unlocked((uint32_t)cur_col * cell_w, (uint32_t)old_screen_row * cell_h, cell_w, cell_h);
        }

        cur_col++;
        if (cur_col >= cols)
        {
            cur_col = 0;
            cur_row++;
            if (cur_row >= visible_rows && !need_full_redraw) scrolled_one = true;
        }
    }

    // Shift scrollback text history when all 200 lines are full
    if (cur_row >= CONSOLE_SCROLLBACK_LINES)
    {
        memcpy(text_buffer[0], text_buffer[1],
               (CONSOLE_SCROLLBACK_LINES - 1u) * CONSOLE_MAX_COLS);
        memset(text_buffer[CONSOLE_SCROLLBACK_LINES - 1u], ' ', CONSOLE_MAX_COLS);
        cur_row = CONSOLE_SCROLLBACK_LINES - 1u;
        cur_col = 0;
        if (!need_full_redraw) scrolled_one = true;
    }

    if (target_surface != 0)
    {
        if (need_full_redraw)
        {
            redraw_all_unlocked();
        }
        else if (scrolled_one)
        {
            uint32_t grid_w = cols * cell_w;
            uint32_t grid_h = visible_rows * cell_h;

            gfx_scroll_rect_up(target_surface, 0, 0, grid_w, grid_h, cell_h, bg_color);
            draw_cell_unlocked(visible_rows - 1u, cur_col, text_buffer[cur_row][cur_col], true);
            mark_dirty_unlocked(0, 0, grid_w, grid_h);
        }
        else
        {
            size_t new_screen_row = (cur_row >= visible_rows) ? (visible_rows - 1u) : cur_row;
            draw_cell_unlocked(new_screen_row, cur_col, text_buffer[cur_row][cur_col], true);
            mark_dirty_unlocked((uint32_t)cur_col * cell_w, (uint32_t)new_screen_row * cell_h, cell_w, cell_h);
        }
    }
}

void console_attach(surface_t* surface, console_dirty_cb_t on_dirty)
{
    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    target_surface = surface;
    dirty_cb       = on_dirty;
    dirty_valid    = false;

    if (target_surface != 0)
    {
        cell_w = font_get_width();
        cell_h = font_get_height();

        cols = target_surface->width / cell_w;
        if (cols == 0) cols = 1u;
        if (cols > CONSOLE_MAX_COLS) cols = CONSOLE_MAX_COLS;

        visible_rows = target_surface->height / cell_h;
        if (visible_rows == 0) visible_rows = 1u;
        if (visible_rows > CONSOLE_SCROLLBACK_LINES) visible_rows = CONSOLE_SCROLLBACK_LINES;

        if (cur_col >= cols) cur_col = cols - 1u;

        redraw_all_unlocked();
        flush_dirty_unlocked();
    }

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
}

void console_set_colors(uint32_t fg, uint32_t bg)
{
    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    fg_color = fg;
    bg_color = bg;
    if (target_surface != 0)
    {
        redraw_all_unlocked();
        flush_dirty_unlocked();
    }

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
}

void console_redraw(void)
{
    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    if (target_surface != 0)
    {
        redraw_all_unlocked();
        flush_dirty_unlocked();
    }

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
}

void clear_screen(void)
{
    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    memset(text_buffer, ' ', sizeof(text_buffer));
    cur_row       = 0;
    cur_col       = 0;
    scroll_offset = 0;

    if (target_surface != 0)
    {
        redraw_all_unlocked();
        flush_dirty_unlocked();
    }

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
}

void console_scroll_up(size_t lines)
{
    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    size_t max_offset = get_bottom_view_start();
    if (scroll_offset + lines > max_offset) scroll_offset = max_offset;
    else scroll_offset += lines;

    if (target_surface != 0)
    {
        redraw_all_unlocked();
        flush_dirty_unlocked();
    }

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
}

void console_scroll_down(size_t lines)
{
    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    if (scroll_offset < lines) scroll_offset = 0;
    else scroll_offset -= lines;

    if (target_surface != 0)
    {
        redraw_all_unlocked();
        flush_dirty_unlocked();
    }

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
}

void putchar(char c)
{
    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    putchar_unlocked(c);
    flush_dirty_unlocked();

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
}

void print(const char* s)
{
    if (s == 0) return;

    uint32_t eflags;
    __asm__ volatile ("pushfl; pop %0; cli" : "=r"(eflags) : : "memory");

    for (size_t i = 0; s[i] != '\0'; ++i)
    {
        putchar_unlocked(s[i]);
    }
    flush_dirty_unlocked();

    __asm__ volatile ("push %0; popfl" : : "r"(eflags) : "memory", "cc");
}
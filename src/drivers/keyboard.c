#include "keyboard.h"

// 1D array of single characters mapping to Scancode Set 1
const char keyboard_map[128] = {
    0,    27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    '-',  '=', '\b',  // 0x0E is Backspace
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', // 0x1C is Enter
    0,    // 0x1D is Left Control
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,    // 0x2A is Left Shift
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0,    // 0x36 is Right Shift
    '*',
    0,    // 0x38 is Left Alt
    ' ',  // 0x39 is Space
    0,    // 0x3A is Caps Lock
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x3B - 0x44: F1 through F10
    0,    // 0x45 is NumLock
    0,    // 0x46 is ScrollLock
    '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.', // 0x47 - 0x53: Keypad
    0, 0, 0, // 0x54 - 0x56: Empty/Undefined
    0,    // 0x57 is F11
    0     // 0x58 is F12
};
const char keyboard_map_shift[128] = {
    0,    27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_',  '+', '\b',  
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,    
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0,    
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
    0, '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    
    '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.', 
    0, 0, 0, 0, 0     
};

static bool shift_pressed = false;
static bool caps_lock = false;

void keyboard_handler() 
{
    uint8_t scancode = inb(0x60);

    if (scancode == 0xAA || scancode == 0xB6)
        shift_pressed = false;

    else if (scancode < 0x80) {
        
        if (scancode == 0x2A || scancode == 0x36) shift_pressed = true;
        else if (scancode == 0x3A) caps_lock = !caps_lock;
        else if (scancode == 0x3B) clear_screen();
        else {
            char c = 0;
            if (shift_pressed) c = keyboard_map_shift[scancode];
            else c = keyboard_map[scancode];
            if (caps_lock) {
                if (c >= 'a' && c <= 'z') {
                    c = c - 32; 
                } else if (c >= 'A' && c <= 'Z') {
                    c = c + 32; 
                }
            }
            if (c != 0) {
                putchar(c);
            }
        }
    }
    outb(0x20, 0x20); 
}
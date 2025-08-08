#include <cstdint>
#include <cstddef>
#include <limine.h>
#include "fb.h"
#include "font.h"
#include "../lib/sys_utils.h"
#include "../lib/memory/memory.h"

// Global framebuffer pointer, initialized on boot.
limine_framebuffer *global_framebuffer;

// --- Text Console State ---
// These static variables track the cursor position for printing text.
static size_t cursor_x = 0;
static size_t cursor_y = 0;
const size_t FONT_WIDTH = 8;
const size_t FONT_HEIGHT = 8;

// --- Private Functions ---

// Scrolls the entire screen up by one character row (FONT_HEIGHT).
static void scroll_screen() {
    // Calculate the number of bytes to move.
    size_t scroll_bytes = FONT_HEIGHT * global_framebuffer->pitch;
    
    // The destination is the start of the framebuffer.
    void* dest = global_framebuffer->address;
    
    // The source is one character row down from the start.
    const void* src = (uint8_t*)global_framebuffer->address + scroll_bytes;
    
    // The total size to move is the entire framebuffer minus one row.
    size_t size_to_move = global_framebuffer->height * global_framebuffer->pitch - scroll_bytes;

    // Move the screen content up.
    memmove(dest, src, size_to_move);

    // Clear the last line on the screen.
    void* last_line = (uint8_t*)global_framebuffer->address + (global_framebuffer->height - FONT_HEIGHT) * global_framebuffer->pitch;
    memset(last_line, 0, scroll_bytes);
}

// --- Public Functions ---

// Function to draw a single pixel to the given buffer
void draw_pixel_to_buffer(uint32_t* buffer, std::size_t width, std::size_t x, std::size_t y, uint32_t color) {
    if (x >= width || y >= global_framebuffer->height) return;
    buffer[y * width + x] = color;
}

// Function to draw a filled rectangle with bounds checking to the given buffer
void draw_rect_to_buffer(uint32_t* buffer, std::size_t width, std::size_t height, int x, int y, int rect_width, int rect_height, uint32_t color) {
    for (int j = 0; j < rect_height; ++j) {
        for (int i = 0; i < rect_width; ++i) {
            int draw_x = x + i;
            int draw_y = y + j;
            if (draw_x >= 0 && (std::size_t)draw_x < width && draw_y >= 0 && (std::size_t)draw_y < height) {
                draw_pixel_to_buffer(buffer, width, draw_x, draw_y, color);
            }
        }
    }
}

// Prints a single character and handles cursor advancement and scrolling.
void print_char(char c, uint32_t color) {
    if (global_framebuffer == nullptr) return;

    // Handle newline characters.
    if (c == '\n') {
        cursor_x = 0;
        cursor_y += FONT_HEIGHT;
    } else if (c >= 32 && c <= 126) {
        // Draw the character glyph.
        const uint8_t *glyph = font8x8_basic[c - 32];
        for (size_t row = 0; row < FONT_HEIGHT; ++row) {
            for (size_t col = 0; col < FONT_WIDTH; ++col) {
                if ((glyph[row] >> col) & 1) {
                    draw_pixel_to_buffer((uint32_t*)global_framebuffer->address, global_framebuffer->width, cursor_x + col, cursor_y + row, color);
                }
            }
        }
        // Advance the cursor.
        cursor_x += FONT_WIDTH;
    }

    // Wrap the cursor to the next line if it goes off the right side of the screen.
    if (cursor_x + FONT_WIDTH > global_framebuffer->width) {
        cursor_x = 0;
        cursor_y += FONT_HEIGHT;
    }

    // Scroll the screen if the cursor goes off the bottom.
    if (cursor_y + FONT_HEIGHT > global_framebuffer->height) {
        scroll_screen();
        cursor_y -= FONT_HEIGHT;
    }
}

// Prints a string by calling print_char for each character.
void print_string(const char* str, uint32_t color) {
    for (size_t i = 0; str[i] != '\0'; ++i) {
        print_char(str[i], color);
    }
}

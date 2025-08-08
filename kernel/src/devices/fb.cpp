#include <cstdint>
#include <cstddef>
#include <limine.h>
#include "fb.h"
#include "font.h"
#include "../lib/sys_utils.h"


// Global framebuffer pointer, initialized on boot.
limine_framebuffer *global_framebuffer;


// Function to draw a single pixel to the given buffer
void draw_pixel_to_buffer(uint32_t* buffer, std::size_t width, std::size_t x, std::size_t y, uint32_t color) {
    if (x >= width) return;
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

// Function to print a string to the framebuffer
void print_string(limine_framebuffer *fb, const char* str, size_t x, size_t y, uint32_t color) {
    if (fb == nullptr) return;
    for (size_t i = 0; str[i] != '\0'; ++i) {
        if (str[i] >= 32 && str[i] <= 126) {
            const uint8_t *glyph = font8x8_basic[str[i] - 32];
            for (size_t row = 0; row < 8; ++row) {
                for (size_t col = 0; col < 8; ++col) {
                    if ((glyph[row] >> (7 - col)) & 1) {
                        draw_pixel_to_buffer((uint32_t*)fb->address, fb->width, x + col + (i * 8), y + row, color);
                    }
                }
            }
        }
    }
}

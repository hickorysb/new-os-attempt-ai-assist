#pragma once

#include <cstdint>
#include <cstddef>
#include <limine.h>

extern const uint8_t font8x8_basic[128][8];
extern limine_framebuffer *global_framebuffer;


// Function prototypes for our framebuffer driver
void draw_pixel_to_buffer(uint32_t* buffer, std::size_t width, std::size_t x, std::size_t y, uint32_t color);
void draw_rect_to_buffer(uint32_t* buffer, std::size_t width, std::size_t height, int x, int y, int rect_width, int rect_height, uint32_t color);
void print_string(limine_framebuffer *fb, const char* str, size_t x, size_t y, uint32_t color);
#pragma once

#include <cstdint>
#include <cstddef>
#include <limine.h>

// Global framebuffer pointer, initialized on boot.
extern limine_framebuffer *global_framebuffer;

// Function prototypes for our framebuffer driver.
void draw_pixel_to_buffer(uint32_t* buffer, std::size_t width, std::size_t x, std::size_t y, uint32_t color);
void draw_rect_to_buffer(uint32_t* buffer, std::size_t width, std::size_t height, int x, int y, int rect_width, int rect_height, uint32_t color);

// Prints a single character to the screen at the current cursor position,
// handling scrolling and newlines.
void print_char(char c, uint32_t color);

// Prints a string to the screen using the new cursor and scrolling system.
void print_string(const char* str, uint32_t color);

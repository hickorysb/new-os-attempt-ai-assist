#pragma once

#include <cstdint>
#include <cstddef>
#include <limine.h>

// --- Global State ---
// Global framebuffer pointer, initialized on boot.
extern limine_framebuffer *global_framebuffer;
// Pointer to our backbuffer.
extern uint32_t* back_buffer;
// Flag to indicate if double buffering is active.
extern bool double_buffer_enabled;

// --- Font Constants ---
// Make font dimensions available to other files.
extern const size_t FONT_WIDTH;
extern const size_t FONT_HEIGHT;


// --- Function Prototypes ---

// Initializes the backbuffer for double buffering.
void fb_init_double_buffer();

// Swaps the backbuffer to the front (visible) buffer.
void fb_swap_buffers();

// Clears the active buffer (backbuffer if enabled, otherwise front buffer).
void fb_clear_buffer(uint32_t color);

// --- Console Functions (Scrolling) ---
// These functions operate on a static cursor and will scroll the screen.
void fb_print_char(char c, uint32_t color);
void fb_print_string(const char* str, uint32_t color);

// --- Direct Drawing Functions ---
// These functions draw directly to the active buffer at specified coordinates.
void fb_draw_pixel(std::size_t x, std::size_t y, uint32_t color);
void fb_draw_rect(int x, int y, int rect_width, int rect_height, uint32_t color);
void fb_draw_char_at(char c, int x, int y, uint32_t color);
void fb_draw_string_at(const char* str, int x, int y, uint32_t color);

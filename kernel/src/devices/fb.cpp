#include <cstdint>
#include <cstddef>
#include <limine.h>
#include "fb.h"
#include "font.h"
#include "../lib/sys_utils.h"
#include "../lib/memory/memory.h"
#include "../memory/pmm.h"
#include "../memory/vmm.h"

// --- Global State ---
limine_framebuffer *global_framebuffer = nullptr;
uint32_t* back_buffer = nullptr;
bool double_buffer_enabled = false;
static size_t back_buffer_size_bytes = 0;

// --- Text Console State ---
static size_t console_cursor_x = 0;
static size_t console_cursor_y = 0;
// These are now global definitions, not static, so they can be accessed externally.
const size_t FONT_WIDTH = 8;
const size_t FONT_HEIGHT = 8;

// --- Private Helper Functions ---

// Gets a pointer to the currently active buffer.
static uint32_t* get_active_buffer() {
    if (double_buffer_enabled && back_buffer != nullptr) {
        return back_buffer;
    }
    // Before double buffering is enabled, we draw directly to the visible framebuffer.
    return (uint32_t*)global_framebuffer->address;
}

// Scrolls the active buffer up by one character row.
static void scroll_screen() {
    if (global_framebuffer == nullptr) return;
    
    uint32_t* active_buffer = get_active_buffer();
    size_t scroll_bytes = FONT_HEIGHT * global_framebuffer->pitch;
    
    void* dest = active_buffer;
    const void* src = (uint8_t*)active_buffer + scroll_bytes;
    size_t size_to_move = (global_framebuffer->height * global_framebuffer->pitch) - scroll_bytes;

    memmove(dest, src, size_to_move);

    void* last_line = (uint8_t*)active_buffer + (global_framebuffer->height - FONT_HEIGHT) * global_framebuffer->pitch;
    memset(last_line, 0, scroll_bytes);
}

// --- Public Functions ---

void fb_init_double_buffer() {
    if (global_framebuffer == nullptr) return;

    back_buffer_size_bytes = global_framebuffer->height * global_framebuffer->pitch;
    size_t num_pages = (back_buffer_size_bytes + PAGE_SIZE - 1) / PAGE_SIZE;

    // Define a virtual address for our backbuffer. This is an arbitrary, high address.
    uintptr_t back_buffer_virt_addr = 0xFFFF810000000000; 

    // Get the current PML4 table to map our new pages.
    uint64_t current_pml4_phys;
    asm volatile ("mov %%cr3, %0" : "=r"(current_pml4_phys));
    extern volatile struct limine_hhdm_request hhdm_request;
    PageTable* current_pml4_virt = (PageTable*)(current_pml4_phys + hhdm_request.response->offset);

    // Allocate and map each page for the backbuffer.
    for (size_t i = 0; i < num_pages; i++) {
        void* phys_frame = pmm_alloc_frame();
        if (!phys_frame) {
            fb_print_string("Failed to allocate physical memory for backbuffer page!\n", 0xFF0000);
            // A real implementation should free the pages allocated so far.
            return;
        }
        uintptr_t virt_addr = back_buffer_virt_addr + (i * PAGE_SIZE);
        vmm_map_page(current_pml4_virt, virt_addr, (uintptr_t)phys_frame, PTE_PRESENT | PTE_WRITABLE);
    }

    back_buffer = (uint32_t*)back_buffer_virt_addr;
    double_buffer_enabled = true;
    fb_print_string("Double buffering enabled.\n", 0x00FF00);
}

void fb_swap_buffers() {
    if (!double_buffer_enabled || back_buffer == nullptr) return;
    memcpy(global_framebuffer->address, back_buffer, back_buffer_size_bytes);
}

void fb_clear_buffer(uint32_t color) {
    if (global_framebuffer == nullptr) return;
    uint32_t* active_buffer = get_active_buffer();
    size_t buffer_size_dwords = (global_framebuffer->height * global_framebuffer->pitch) / 4;
    
    // A 32-bit version of memset.
    for (size_t i = 0; i < buffer_size_dwords; i++) {
        active_buffer[i] = color;
    }
}

// --- Direct Drawing Functions ---

void fb_draw_pixel(std::size_t x, std::size_t y, uint32_t color) {
    if (global_framebuffer == nullptr || x >= global_framebuffer->width || y >= global_framebuffer->height) return;
    uint32_t* active_buffer = get_active_buffer();
    active_buffer[y * (global_framebuffer->pitch / 4) + x] = color;
}

void fb_draw_rect(int x, int y, int rect_width, int rect_height, uint32_t color) {
    for (int j = 0; j < rect_height; ++j) {
        for (int i = 0; i < rect_width; ++i) {
            int draw_x = x + i;
            int draw_y = y + j;
            // Bounds checking is handled by fb_draw_pixel.
            fb_draw_pixel(draw_x, draw_y, color);
        }
    }
}

void fb_draw_char_at(char c, int x, int y, uint32_t color) {
    if (global_framebuffer == nullptr || c < 32 || c > 126) return;

    const uint8_t *glyph = font8x8_basic[c - 32];
    for (size_t row = 0; row < FONT_HEIGHT; ++row) {
        for (size_t col = 0; col < FONT_WIDTH; ++col) {
            if ((glyph[row] >> col) & 1) {
                fb_draw_pixel(x + col, y + row, color);
            }
        }
    }
}

void fb_draw_string_at(const char* str, int x, int y, uint32_t color) {
    for (size_t i = 0; str[i] != '\0'; ++i) {
        fb_draw_char_at(str[i], x + (i * FONT_WIDTH), y, color);
    }
}

// --- Console Functions ---

void fb_print_char(char c, uint32_t color) {
    if (global_framebuffer == nullptr) return;

    if (c == '\n') {
        console_cursor_x = 0;
        console_cursor_y += FONT_HEIGHT;
    } else if (c >= 32 && c <= 126) {
        fb_draw_char_at(c, console_cursor_x, console_cursor_y, color);
        console_cursor_x += FONT_WIDTH;
    }

    if (console_cursor_x + FONT_WIDTH > global_framebuffer->width) {
        console_cursor_x = 0;
        console_cursor_y += FONT_HEIGHT;
    }

    if (console_cursor_y + FONT_HEIGHT > global_framebuffer->height) {
        scroll_screen();
        console_cursor_y -= FONT_HEIGHT;
    }
}

void fb_print_string(const char* str, uint32_t color) {
    for (size_t i = 0; str[i] != '\0'; ++i) {
        fb_print_char(str[i], color);
    }
}

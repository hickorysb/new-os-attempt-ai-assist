#include <cstdint>
#include <cstddef>
#include <limine.h>
#include <cpuid.h>
#include "devices/cpu_config.h"
#include "lib/sys_utils.h"
#include "lib/memory/memory.h"
#include "devices/fb.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "interrupts/idt.h"
#include "devices/keyboard.h"

// Set the base revision to 3, supported by the Limine boot protocol.
LIMINE_BASE_REVISION(3);

// --- Limine Requests ---
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
    .response = nullptr
};

static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
    .response = nullptr
};

volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0,
    .response = nullptr
};

static volatile struct limine_executable_address_request executable_address_request = {
    .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST,
    .revision = 0,
    .response = nullptr
};

// Halt and catch fire function.
namespace {
void hcf() {
    asm ("cli"); // Disable interrupts before halting
    for (;;) {
        asm ("hlt");
    }
}
}

// C++ ABI stubs.
extern "C" {
    int __cxa_atexit(void (*)(void *), void *, void *) { return 0; }
    void __cxa_pure_virtual() { hcf(); }
    void *__dso_handle;
}

// Extern declarations for global constructors array.
extern void (*__init_array[])();
extern void (*__init_array_end[])();

// Kernel entry point
extern "C" void kmain() {
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
    }

    for (std::size_t i = 0; &__init_array[i] != __init_array_end; i++) {
        __init_array[i]();
    }
    
    if (framebuffer_request.response == nullptr
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }
    global_framebuffer = framebuffer_request.response->framebuffers[0];

    fb_print_string("Framebuffer initialized.\n", 0x00FFFFFF);

    if (hhdm_request.response == nullptr) {
        fb_print_string("ERROR: Failed to get HHDM response.\n", 0x00FF0000);
        hcf();
    }

    if (memmap_request.response == nullptr) {
        fb_print_string("ERROR: Failed to get memory map.\n", 0x00FF0000);
        hcf();
    }

    if (executable_address_request.response == nullptr) {
        fb_print_string("ERROR: Failed to get kernel address response.\n", 0x00FF0000);
        hcf();
    }

    uint32_t eax, ebx, ecx, edx;
    if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx) || !(edx & (1 << 0))) {
        hcf(); // No FPU
    }
    enable_fpu_sse();

    pmm_init(memmap_request.response, hhdm_request.response);
    vmm_init(memmap_request.response, hhdm_request.response, executable_address_request.response);
    fb_init_double_buffer();
    
    // Initialize the interrupt descriptor table and keyboard.
    idt_init();
    keyboard_init();

    // Enable interrupts now that all handlers are set up.
    asm volatile ("sti");

    fb_print_string("\n--- Keyboard Test ---\n", 0x0000FFFF);
    fb_draw_string_at("Type something!", 10, global_framebuffer->height - 40, 0x00FFFFFF);
    fb_swap_buffers(); // Show the initial message

    // Simple loop to echo keyboard input.
    for (;;) {
        char c = keyboard_getchar();
        if (c != 0) {
            // Print the character to the scrolling console part of the screen.
            fb_print_char(c, 0xFFFFFF);
            // Also update the static text at the bottom.
            fb_draw_rect(0, global_framebuffer->height - 20, global_framebuffer->width, 20, 0x000000); // Clear old text
            fb_draw_string_at("You typed: ", 10, global_framebuffer->height - 20, 0x00FFFFFF);
            fb_draw_char_at(c, 11 * FONT_WIDTH, global_framebuffer->height - 20, 0x0000FF00);
            fb_swap_buffers();
        }
    }
}

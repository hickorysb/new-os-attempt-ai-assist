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
#include "interrupts/interrupts.h"
#include "devices/keyboard.h"
#include "lib/io.h" // Include io.h for inb

LIMINE_BASE_REVISION(3);

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

namespace {
void hcf() {
    asm ("cli");
    for (;;) {
        asm ("hlt");
    }
}
}

extern "C" {
    int __cxa_atexit(void (*)(void *), void *, void *) { return 0; }
    void __cxa_pure_virtual() { hcf(); }
    void *__dso_handle;
}

extern void (*__init_array[])();
extern void (*__init_array_end[])();

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

    if (hhdm_request.response == nullptr) { hcf(); }
    if (memmap_request.response == nullptr) { hcf(); }
    if (executable_address_request.response == nullptr) { hcf(); }

    uint32_t eax, ebx, ecx, edx;
    if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx) || !(edx & (1 << 0))) {
        hcf(); // No FPU
    }
    enable_fpu_sse();

    pmm_init(memmap_request.response, hhdm_request.response);
    vmm_init(memmap_request.response, hhdm_request.response, executable_address_request.response);
    fb_init_double_buffer();
    
    // Initialize GDT, IDT, and PICs
    interrupts_init();
    fb_swap_buffers();
    
    // Initialize the keyboard driver
    keyboard_init();
    fb_swap_buffers();

    // After keyboard init, flush the PS/2 data port one last time to clear any pending IRQs.
    // This is a crucial step to prevent an immediate interrupt after 'sti'.
    fb_print_string("Flushing PS/2 data port before enabling interrupts.\n", 0x00FFFF00);
    fb_swap_buffers();
    while (inb(0x64) & 1) {
        inb(0x60);
    }
    fb_print_string("PS/2 data port flushed. Enabling interrupts.\n", 0x00FFFF00);
    fb_swap_buffers();
    
    // Enable interrupts now that everything is set up.
    asm volatile ("sti");
    fb_print_string("Interrupts enabled.\n", 0x00FFFF00);
    fb_swap_buffers();

    fb_print_string("\n--- Keyboard Test ---\n", 0x0000FFFF);
    fb_draw_string_at("Type something!", 10, global_framebuffer->height - 40, 0x00FFFFFF);
    fb_swap_buffers(); 

    for (;;) {
        char c;
        while ((c = keyboard_getchar()) != 0) {
            fb_print_char(c, 0xFFFFFF);
        }
        fb_swap_buffers();
        asm ("hlt");
    }
}

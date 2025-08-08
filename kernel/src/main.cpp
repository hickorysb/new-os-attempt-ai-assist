#include <cstdint>
#include <cstddef>
#include <limine.h>
#include <cpuid.h>
#include "devices/cpu_config.h"
#include "lib/sys_utils.h"
#include "devices/fb.h"

// Set the base revision to 3, supported by the Limine boot protocol.
LIMINE_BASE_REVISION(3);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent.
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
    .response = nullptr // filled by bootloader
};

// GCC and Clang reserve the right to generate calls to the following
// 4 functions even if they are not directly called.
// Implement them as the C specification mandates.
// DO NOT remove or rename these functions, or stuff will eventually break!
// They CAN be moved to a different .cpp file.
extern "C" {
void *memcpy(void *__restrict dest, const void *__restrict src, std::size_t n) {
    std::uint8_t *__restrict pdest = static_cast<std::uint8_t *__restrict>(dest);
    const std::uint8_t *__restrict psrc = static_cast<const std::uint8_t *__restrict>(src);
    for (std::size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }
    return dest;
}

void *memset(void *s, int c, std::size_t n) {
    std::uint8_t *p = static_cast<std::uint8_t *>(s);
    for (std::size_t i = 0; i < n; i++) {
        p[i] = static_cast<uint8_t>(c);
    }
    return s;
}

void *memmove(void *dest, const void *src, std::size_t n) {
    std::uint8_t *pdest = static_cast<std::uint8_t *>(dest);
    const std::uint8_t *psrc = static_cast<const std::uint8_t *>(src);
    if (src > dest) {
        for (std::size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if (src < dest) {
        for (std::size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }
    return dest;
}

int memcmp(const void *s1, const void *s2, std::size_t n) {
    const std::uint8_t *p1 = static_cast<const std::uint8_t *>(s1);
    const std::uint8_t *p2 = static_cast<const std::uint8_t *>(s2);
    for (std::size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }
    return 0;
}
}

// Halt and catch fire function.
namespace {
void hcf() {
    for (;;) {
#if defined (__x86_64__)
        asm ("hlt");
#elif defined (__aarch64__) || defined (__riscv)
        asm ("wfi");
#elif defined (__loongarch64)
        asm ("idle 0");
#endif
    }
}
}

// The following stubs are required by the Itanium C++ ABI (the one we use,
// regardless of the "Itanium" nomenclature).
// Like the memory functions above, these stubs can be moved to a different .cpp file,
// but should not be removed, unless you know what you are doing.
extern "C" {
    int __cxa_atexit(void (*)(void *), void *, void *) { return 0; }
    void __cxa_pure_virtual() { hcf(); }
    void *__dso_handle;
}

// Extern declarations for global constructors array.
extern void (*__init_array[])();
extern void (*__init_array_end[])();

// Animation function for a bouncing rectangle with double buffering
void run_bouncing_animation() {
    int width = (int)global_framebuffer->width;
    int height = (int)global_framebuffer->height;
    
    // Allocate a back buffer with the exact size of the framebuffer.
    // Use a static buffer to avoid stack overflow or dynamic allocation issues.
    static uint32_t back_buffer[4096 * 2160];
    
    // Rectangle properties
    float fx = 0.0f, fy = 0.0f;
    float dx = 2.0f, dy = 2.0f;
    int rect_width = 100;
    int rect_height = 50;
    
    // Pre-shifted color values
    uint32_t colors[] = {
        (static_cast<uint32_t>(255) << global_framebuffer->red_mask_shift) |
        (static_cast<uint32_t>(0) << global_framebuffer->green_mask_shift) |
        (static_cast<uint32_t>(0) << global_framebuffer->blue_mask_shift), // Red
        (static_cast<uint32_t>(0) << global_framebuffer->red_mask_shift) |
        (static_cast<uint32_t>(255) << global_framebuffer->green_mask_shift) |
        (static_cast<uint32_t>(0) << global_framebuffer->blue_mask_shift), // Green
        (static_cast<uint32_t>(0) << global_framebuffer->red_mask_shift) |
        (static_cast<uint32_t>(0) << global_framebuffer->green_mask_shift) |
        (static_cast<uint32_t>(255) << global_framebuffer->blue_mask_shift), // Blue
        (static_cast<uint32_t>(255) << global_framebuffer->red_mask_shift) |
        (static_cast<uint32_t>(255) << global_framebuffer->green_mask_shift) |
        (static_cast<uint32_t>(0) << global_framebuffer->blue_mask_shift), // Yellow
        (static_cast<uint32_t>(255) << global_framebuffer->red_mask_shift) |
        (static_cast<uint32_t>(0) << global_framebuffer->green_mask_shift) |
        (static_cast<uint32_t>(255) << global_framebuffer->blue_mask_shift), // Magenta
        (static_cast<uint32_t>(0) << global_framebuffer->red_mask_shift) |
        (static_cast<uint32_t>(255) << global_framebuffer->green_mask_shift) |
        (static_cast<uint32_t>(255) << global_framebuffer->blue_mask_shift), // Cyan
    };
    std::size_t current_color_idx = 0;
    uint32_t current_color = colors[current_color_idx];

    while (true) {
        memset(back_buffer, 0, width * height * 4);
        
        fx += dx;
        fy += dy;

        bool bounced = false;
        if ((fx + rect_width) > width || fx < 0) {
            dx = -dx;
            if (fx < 0) fx = 0;
            if ((fx + rect_width) > width) fx = (float)width - (float)rect_width;
            bounced = true;
        }
        if ((fy + rect_height) > height || fy < 0) {
            dy = -dy;
            if (fy < 0) fy = 0;
            if ((fy + rect_height) > height) fy = (float)height - (float)rect_height;
            bounced = true;
        }

        if (bounced) {
            current_color_idx = (current_color_idx + 1) % (sizeof(colors) / sizeof(colors[0]));
            current_color = colors[current_color_idx];
        }
        
        draw_rect_to_buffer(back_buffer, width, height, (int)fx, (int)fy, rect_width, rect_height, current_color);
        
        for (std::size_t i = 0; i < global_framebuffer->height; ++i) {
            memcpy(
                (void *)((uintptr_t)global_framebuffer->address + i * global_framebuffer->pitch),
                (void *)&back_buffer[i * width],
                width * 4
            );
        }

        // A simple spin loop for a small delay to make the animation visible.
        volatile long long counter = 0;
        // Increased delay value to slow down animation.
        while (counter < 50000000) {
            counter = counter + 1;
        }
    }
}


// The following will be our kernel's entry point.
// If renaming kmain() to something else, make sure to change the
// linker script accordingly.
extern "C" void kmain() {
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
    }

    for (std::size_t i = 0; &__init_array[i] != __init_array_end; i++) {
        __init_array[i]();
    }
    
    // FPU/SSE Check and setup
    uint32_t eax, ebx, ecx, edx;
    if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        hcf();
    }
    bool fpu_present = (edx & (1 << 0)) != 0;
    
    if (!fpu_present) {
        // Since we don't have a framebuffer pointer yet, we can't print.
        // We'll just halt the system silently.
        hcf();
    }
    enable_fpu_sse();

    // Ensure we got a framebuffer.
    if (framebuffer_request.response == nullptr
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    // Fetch the first framebuffer.
    global_framebuffer = framebuffer_request.response->framebuffers[0];

    // Run the animation
    run_bouncing_animation();
    
    // The animation loop is infinite, so the code below is not reachable.
    // However, it's good practice to keep it for potential future changes.
    // If the animation loop were to break, the kernel would proceed here.

    // Animation is done, clear the screen and print information
    memset((void *)global_framebuffer->address, 0x00, global_framebuffer->height * global_framebuffer->pitch); // Clear to black

    // Print framebuffer dimensions
    const int start_x = 10;
    int start_y = 10;
    char buffer[32];
    uint32_t text_color = 0x00FFFFFF; // White text

    print_string(global_framebuffer, "FB Dimensions:", start_x, start_y, text_color);
    start_y += 16;
    print_string(global_framebuffer, "Width: ", start_x, start_y, text_color);
    itoa(global_framebuffer->width, buffer);
    print_string(global_framebuffer, buffer, start_x + 8*8, start_y, text_color);

    start_y += 16;
    print_string(global_framebuffer, "Height: ", start_x, start_y, text_color);
    itoa(global_framebuffer->height, buffer);
    print_string(global_framebuffer, buffer, start_x + 8*8, start_y, text_color);

    start_y += 16;
    print_string(global_framebuffer, "Pitch: ", start_x, start_y, text_color);
    itoa(global_framebuffer->pitch, buffer);
    print_string(global_framebuffer, buffer, start_x + 8*8, start_y, text_color);

    start_y += 16;
    print_string(global_framebuffer, "BPP: ", start_x, start_y, text_color);
    itoa(global_framebuffer->bpp, buffer);
    print_string(global_framebuffer, buffer, start_x + 8*4, start_y, text_color);
    
    start_y += 32;
    print_string(global_framebuffer, "Shutting down in 5 seconds...", start_x, start_y, text_color);

    delay(5000000000);

    hcf();
}

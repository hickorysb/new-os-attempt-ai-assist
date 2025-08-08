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

// Removed the non-existent limine_stack_request.

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

    if (hhdm_request.response == nullptr) {
        print_string("ERROR: Failed to get HHDM response.\n", 0x00FF0000);
        hcf();
    }

    if (memmap_request.response == nullptr) {
        print_string("ERROR: Failed to get memory map.\n", 0x00FF0000);
        hcf();
    }

    if (executable_address_request.response == nullptr) {
        print_string("ERROR: Failed to get kernel address response.\n", 0x00FF0000);
        hcf();
    }

    uint32_t eax, ebx, ecx, edx;
    if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx) || !(edx & (1 << 0))) {
        hcf(); // No FPU
    }
    enable_fpu_sse();

    pmm_init(memmap_request.response, hhdm_request.response);
    
    // Removed the non-existent stack_request.response from the call.
    vmm_init(memmap_request.response, hhdm_request.response, executable_address_request.response);

    print_string("\n--- PMM Test ---\n", 0x0000FFFF);
    char buffer[32];

    void* frame1 = pmm_alloc_frame();
    print_string("Allocated frame 1 at: ", 0x00FFFFFF);
    u64_to_str((uint64_t)frame1, buffer);
    print_string(buffer, 0x00FFFFFF);
    print_string("\n", 0x00FFFFFF);

    void* frame2 = pmm_alloc_frame();
    print_string("Allocated frame 2 at: ", 0x00FFFFFF);
    u64_to_str((uint64_t)frame2, buffer);
    print_string(buffer, 0x00FFFFFF);
    print_string("\n", 0x00FFFFFF);

    print_string("Freeing frame 1...\n", 0x00FFFFFF);
    pmm_free_frame(frame1);

    void* frame3 = pmm_alloc_frame();
    print_string("Allocated frame 3 at: ", 0x00FFFFFF);
    u64_to_str((uint64_t)frame3, buffer);
    print_string(buffer, 0x00FFFFFF);
    print_string("\n", 0x00FFFFFF);

    void* frame4 = pmm_alloc_frame();
    print_string("Allocated frame 4 at: ", 0x00FFFFFF);
    u64_to_str((uint64_t)frame4, buffer);
    print_string(buffer, 0x00FFFFFF);
    print_string("\n", 0x00FFFFFF);


    print_string("--- PMM Test Complete ---\n", 0x0000FFFF);
    print_string("\nAll systems initialized. Kernel is now idle.\n", 0x0000FF00);

    hcf();
}

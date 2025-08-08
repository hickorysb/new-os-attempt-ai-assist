#include <cstdint>
#include "cpu_config.h"

// Function to enable FPU and SSE registers.
void enable_fpu_sse() {
#if defined (__x86_64__)
    uint64_t cr0;
    asm volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 2); // Clear the EM bit (emulation)
    cr0 |= (1 << 1);  // Set the MP bit (monitor co-processor)
    asm volatile ("mov %0, %%cr0" :: "r"(cr0));

    uint64_t cr4;
    asm volatile ("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 9); // Set the OSFXSR bit (OS support for FXSAVE/FXRSTOR)
    cr4 |= (1 << 10); // Set the OSXMMEXCPT bit (OS support for unmasked SSE exceptions)
    asm volatile ("mov %0, %%cr4" :: "r"(cr4));
#endif
}

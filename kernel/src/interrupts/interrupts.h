#pragma once

#include <cstdint>

// This structure defines the registers that are saved on the stack by our ISR stubs.
// It is passed to the C-level interrupt handlers.
struct registers_t {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    // These are pushed by the CPU automatically on interrupt.
    uint64_t rip, cs, rflags, userrsp, ss;
};

// A type definition for our interrupt handler function pointers.
typedef void (*isr_t)(registers_t*);

/**
 * @brief Registers a handler function for a given interrupt number.
 * @param n The interrupt number (0-255).
 * @param handler A pointer to the function that will handle the interrupt.
 */
void register_interrupt_handler(uint8_t n, isr_t handler);

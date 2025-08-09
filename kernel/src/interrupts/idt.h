#pragma once
#include <cstdint>

// Defines an entry in the IDT for x86_64.
struct IdtEntry {
    uint16_t isr_low;
    uint16_t kernel_cs;
    uint8_t  ist;
    uint8_t  attributes;
    uint16_t isr_mid;
    uint32_t isr_high;
    uint32_t reserved;
} __attribute__((packed));

// A pointer structure for the 'lidt' instruction.
struct IdtPtr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/**
 * @brief Initializes the GDT, IDT, and PICs.
 * This is the central function for setting up all interrupt handling.
 */
void interrupts_init();

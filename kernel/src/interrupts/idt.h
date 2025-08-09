#pragma once

#include <cstdint>

// Defines an entry in the Interrupt Descriptor Table for x86_64.
// This structure tells the CPU where to find the handler for each interrupt.
struct IdtEntry {
    uint16_t isr_low;    // The lower 16 bits of the ISR's address.
    uint16_t kernel_cs;  // The GDT segment selector that the CPU will load into CS before calling the ISR.
    uint8_t  ist;        // Interrupt Stack Table offset.
    uint8_t  attributes; // Type and attribute flags.
    uint16_t isr_mid;    // The middle 16 bits of the ISR's address.
    uint32_t isr_high;   // The upper 32 bits of the ISR's address.
    uint32_t reserved;   // Set to zero.
} __attribute__((packed));

// A pointer structure for the 'lidt' instruction, which loads our IDT.
struct IdtPtr {
    uint16_t limit; // Size of the IDT in bytes - 1.
    uint64_t base;  // The linear address of the IDT.
} __attribute__((packed));


/**
 * @brief Initializes the Interrupt Descriptor Table and loads it.
 */
void idt_init();

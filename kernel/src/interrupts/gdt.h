#pragma once
#include <cstdint>

// Defines a single entry in the GDT.
// We use the packed attribute to prevent the compiler from adding padding.
struct GdtEntry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

// A pointer structure for the 'lgdt' instruction.
struct GdtPtr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/**
 * @brief Initializes and loads our new GDT.
 */
void gdt_init();

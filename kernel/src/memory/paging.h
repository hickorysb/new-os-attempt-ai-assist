#pragma once

#include <cstdint>

// --- Page Table Entry (PTE) Flags ---
// These flags control the properties of a mapped page.
#define PTE_PRESENT (1UL << 0)
#define PTE_WRITABLE (1UL << 1)
#define PTE_USER (1UL << 2)
#define PTE_NO_EXECUTE (1UL << 63)

// The structure for a single entry in a page table.
// This is for a 4-level paging scheme on x86-64.
struct PageTableEntry {
    uint64_t value;

    // Sets the physical address for this entry, preserving the flags.
    void set_address(uint64_t address) {
        value = (address & ~0xFFFUL) | (value & 0xFFFUL);
    }

    // Gets the physical address from this entry.
    uint64_t get_address() {
        return value & ~0xFFFUL;
    }

    // Sets the flags for this entry, preserving the address.
    void set_flags(uint64_t flags) {
        value = (value & ~0xFFFUL) | flags;
    }

    // Gets the flags from this entry.
    uint64_t get_flags() {
        return value & 0xFFFUL;
    }
};

// A Page Table is an array of 512 entries.
struct PageTable {
    PageTableEntry entries[512];
};

/**
 * @brief Invalidates a single page in the TLB.
 * This is necessary after changing a page table entry for an active page map.
 * @param m The virtual address of the page to invalidate.
 */
static inline void invlpg(void* m) {
    asm volatile ("invlpg (%0)" :: "r"(m) : "memory");
}

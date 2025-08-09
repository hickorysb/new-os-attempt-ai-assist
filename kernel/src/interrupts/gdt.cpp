#include "gdt.h"

// The GDT itself. We'll have 3 entries:
// 0: Null descriptor
// 1: Kernel Code Segment (0x08)
// 2: Kernel Data Segment (0x10)
static GdtEntry gdt_entries[3];
static GdtPtr   gdt_ptr;

// External function to load the GDT register (defined in assembly).
extern "C" void gdt_flush(uint64_t);

/**
 * @brief Sets up a single GDT entry.
 */
static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;
    
    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}

/**
 * @brief Initializes the GDT and loads it.
 */
void gdt_init() {
    gdt_ptr.limit = (sizeof(GdtEntry) * 3) - 1;
    gdt_ptr.base  = (uint64_t)&gdt_entries;

    // Null segment
    gdt_set_gate(0, 0, 0, 0, 0);
    // Kernel Code Segment: base=0, limit=0xFFFFFFFF, access=0x9A, granularity=0xCF
    // Access (0x9A): Present(1), Ring 0(00), Code Segment(1), Executable(1), Direction(0), Readable(1), Accessed(0)
    // Granularity (0xCF): Granularity(1), 32-bit(1), 64-bit available(0), AVL(0)
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xAF); 
    // Kernel Data Segment: Same as code segment, but with access=0x92
    // Access (0x92): Present(1), Ring 0(00), Data Segment(1), Direction(0), Writable(1), Accessed(0)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xAF);

    // Load the GDT.
    gdt_flush((uint64_t)&gdt_ptr);
}

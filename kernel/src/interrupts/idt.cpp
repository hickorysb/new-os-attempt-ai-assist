#include "idt.h"
#include "../lib/memory/memory.h"
#include "../devices/fb.h"
#include "../devices/pic.h"

#define IDT_ENTRIES 256

// The Interrupt Descriptor Table itself.
static IdtEntry idt[IDT_ENTRIES];
// A pointer to the IDT that we'll load using 'lidt'.
static IdtPtr idt_ptr;

// The table of ISR stubs defined in our assembly file.
extern "C" {
    extern void* isr_stub_table[];
}

/**
 * @brief Sets a gate (entry) in the IDT.
 * @param n The index of the gate to set (0-255).
 * @param isr_addr The address of the interrupt service routine.
 * @param selector The kernel code segment selector.
 * @param flags The attribute flags for this gate.
 */
static void idt_set_gate(uint8_t n, uint64_t isr_addr, uint16_t selector, uint8_t flags) {
    idt[n].isr_low    = (isr_addr & 0xFFFF);
    idt[n].kernel_cs  = selector;
    idt[n].ist        = 0;
    idt[n].attributes = flags;
    idt[n].isr_mid    = (isr_addr >> 16) & 0xFFFF;
    idt[n].isr_high   = (isr_addr >> 32) & 0xFFFFFFFF;
    idt[n].reserved   = 0;
}

void idt_init() {
    idt_ptr.limit = sizeof(IdtEntry) * IDT_ENTRIES - 1;
    idt_ptr.base  = (uint64_t)&idt;

    memset(&idt, 0, sizeof(IdtEntry) * IDT_ENTRIES);

    // Remap the PIC before we start setting up IRQ handlers.
    pic_init();

    // Loop through the first 48 ISR stubs (32 exceptions + 16 IRQs)
    // and create an IDT entry for each one.
    for (uint8_t i = 0; i < 48; i++) {
        // 0x08 is the kernel code segment selector.
        // 0x8E means the gate is present, has a ring level of 0 (kernel), and is a 64-bit interrupt gate.
        idt_set_gate(i, (uint64_t)isr_stub_table[i], 0x08, 0x8E);
    }
    
    // Load our new IDT.
    asm volatile ("lidt %0" :: "m"(idt_ptr));
    fb_print_string("IDT Initialized.\n", 0x00FFFF00);
}

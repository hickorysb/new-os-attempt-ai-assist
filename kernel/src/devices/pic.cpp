#include "pic.h"
#include "../lib/io.h"

// I/O port addresses for the PICs
#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1
#define PIC_EOI      0x20 // End-of-Interrupt command code

// The ICW (Initialization Command Words) are used to configure the PICs.
#define ICW1_INIT    0x10
#define ICW1_ICW4    0x01
#define ICW4_8086    0x01

void pic_init() {
    // Save masks
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);

    // Start the initialization sequence in cascade mode.
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);

    // Remap the vector offsets.
    // By default, IRQs 0-7 conflict with CPU exceptions 8-15.
    // We remap them to 32-39 and 40-47 respectively.
    outb(PIC1_DATA, 32); // PIC1 master vector offset
    outb(PIC2_DATA, 40); // PIC2 slave vector offset

    // Set up the master-slave relationship.
    outb(PIC1_DATA, 4);  // Tell PIC1 that there is a slave PIC at IRQ2
    outb(PIC2_DATA, 2);  // Tell PIC2 its cascade identity

    // Set 8086/88 (MCS-80/85) mode
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    // Restore saved masks
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

void pic_send_eoi(unsigned char irq) {
    // If the IRQ came from the slave PIC, we need to send an EOI to it.
    if(irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    // Always send an EOI to the master PIC.
    outb(PIC1_COMMAND, PIC_EOI);
}

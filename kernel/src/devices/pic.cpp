#include "pic.h"
#include "../lib/io.h"

// The ICW (Initialization Command Words) are used to configure the PICs.
#define ICW1_INIT    0x10
#define ICW1_ICW4    0x01
#define ICW4_8086    0x01

/**
 * @brief A small delay function for I/O operations.
 * This is often needed for older hardware to have time to process port writes.
 * Port 0x80 is generally unused and safe for this purpose.
 */
static inline void io_wait(void) {
    outb(0x80, 0);
}

void pic_init() {
    // Start the initialization sequence in cascade mode.
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    // Remap the vector offsets.
    // By default, IRQs 0-7 conflict with CPU exceptions 8-15.
    // We remap them to 32-39 and 40-47 respectively.
    outb(PIC1_DATA, 32); // PIC1 master vector offset
    io_wait();
    outb(PIC2_DATA, 40); // PIC2 slave vector offset
    io_wait();

    // Set up the master-slave relationship.
    outb(PIC1_DATA, 4);  // Tell PIC1 that there is a slave PIC at IRQ2
    io_wait();
    outb(PIC2_DATA, 2);  // Tell PIC2 its cascade identity
    io_wait();

    // Set 8086/88 (MCS-80/85) mode
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    // Mask all interrupts on both PICs. We will unmask them as we
    // initialize the drivers. This is more robust than restoring
    // the old masks.
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

void pic_send_eoi(unsigned char irq) {
    // If the IRQ came from the slave PIC, we need to send an EOI to it.
    if(irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    // Always send an EOI to the master PIC.
    outb(PIC1_COMMAND, PIC_EOI);
}

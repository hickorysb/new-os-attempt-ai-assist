#include "interrupts.h"
#include "../devices/fb.h"
#include "../devices/pic.h"
#include "../lib/sys_utils.h"

// An array of function pointers for our interrupt handlers.
static isr_t interrupt_handlers[256];

/**
 * @brief The main interrupt handler, called from the assembly stubs.
 * This function dispatches the interrupt to a registered handler, or prints
 * an error if no handler is registered.
 * @param regs A pointer to the saved registers.
 * @param int_no The interrupt number that occurred.
 */
extern "C" void interrupt_handler(registers_t *regs, uint64_t int_no) {
    // Check if we have a custom handler to run for this interrupt.
    if (interrupt_handlers[int_no] != 0) {
        isr_t handler = interrupt_handlers[int_no];
        handler(regs);
    } else {
        // Default handler if no custom one is registered.
        fb_print_string("Unhandled interrupt: ", 0xFF0000);
        char buffer[32];
        u64_to_str(int_no, buffer);
        fb_print_string(buffer, 0xFF0000);
        fb_print_string("\n", 0xFF0000);
        // A real OS would probably halt here or handle the exception gracefully.
    }

    // If this was a hardware interrupt (IRQ), we need to send an End-of-Interrupt
    // signal to the PIC to let it know we're done handling it.
    if (int_no >= 32 && int_no <= 47) {
        pic_send_eoi(int_no - 32);
    }
}

void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

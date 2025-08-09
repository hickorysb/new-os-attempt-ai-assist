#pragma once

/**
 * @brief Initializes and remaps the master and slave PICs.
 * This is crucial to prevent conflicts between hardware interrupts (IRQs)
 * and CPU exceptions.
 */
void pic_init();

/**
 * @brief Sends an End-of-Interrupt (EOI) signal to the PICs.
 * This must be called at the end of an IRQ handler.
 * @param irq The IRQ number that was handled.
 */
void pic_send_eoi(unsigned char irq);

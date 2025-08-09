#include "keyboard.h"
#include "../lib/io.h"
#include "../interrupts/interrupts.h"
#include "../devices/fb.h"

// A simple circular buffer for keyboard input.
#define KBD_BUFFER_SIZE 256
static char kbd_buffer[KBD_BUFFER_SIZE];
static uint16_t kbd_buffer_read_idx = 0;
static uint16_t kbd_buffer_write_idx = 0;

// Scancode Set 1 to ASCII mapping for a US QWERTY keyboard layout.
// This is a simplified map that doesn't handle shift, ctrl, etc.
static const unsigned char kbd_us[128] = {
      0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
   '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
      0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'','`',   0,
   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*',   0,
    ' ',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0, '-',   0,   0,   0, '+',   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,
};

/**
 * @brief The keyboard interrupt handler. This is called every time a key is pressed.
 * @param regs A pointer to the saved registers (unused in this handler).
 */
static void keyboard_handler(registers_t *regs) {
    (void)regs; // Suppress unused parameter warning.
    
    // Read the scancode from the PS/2 data port.
    uint8_t scancode = inb(0x60);

    // We only handle key presses (scancodes < 128). We ignore key releases.
    if (scancode < 128) {
        char c = kbd_us[scancode];
        if (c != 0) {
            // Add the character to our circular buffer.
            // This is safe from race conditions because interrupts are disabled
            // within the handler.
            kbd_buffer[kbd_buffer_write_idx] = c;
            kbd_buffer_write_idx = (kbd_buffer_write_idx + 1) % KBD_BUFFER_SIZE;
        }
    }
}

void keyboard_init() {
    // Register our handler for IRQ 1 (interrupt 33).
    register_interrupt_handler(33, keyboard_handler);
    fb_print_string("Keyboard Initialized.\n", 0x00FFFF00);
}

char keyboard_getchar() {
    // If the read and write indices are the same, the buffer is empty.
    if (kbd_buffer_read_idx == kbd_buffer_write_idx) {
        return 0;
    }
    
    char c = kbd_buffer[kbd_buffer_read_idx];
    kbd_buffer_read_idx = (kbd_buffer_read_idx + 1) % KBD_BUFFER_SIZE;
    return c;
}

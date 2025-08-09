#include "keyboard.h"
#include "../lib/io.h"
#include "../interrupts/interrupts.h"
#include "../devices/fb.h"
#include "../devices/pic.h"
#include "../lib/sys_utils.h" // Needed for u64_to_str

// --- PS/2 Controller Ports ---
#define KBD_STATUS_PORT 0x64
#define KBD_DATA_PORT   0x60
#define KBD_CMD_PORT    0x64

// --- PS/2 Status Register Bits ---
#define KBD_OUTPUT_BUFFER_FULL 0x01
#define KBD_INPUT_BUFFER_FULL  0x02

// --- PS/2 Controller Commands ---
#define KBD_READ_CONFIG 0x20
#define KBD_WRITE_CONFIG 0x60
#define KBD_ENABLE_SCANNING 0xF4
#define KBD_ENABLE_FIRST_PORT 0xAE
#define KBD_DISABLE_FIRST_PORT 0xAD
#define KBD_SET_TYPEMATIC_RATE_DELAY 0xF3

// --- PS/2 Config Byte Bits ---
#define KBD_FIRST_PORT_INTERRUPT 0x01

// A simple circular buffer for keyboard input.
#define KBD_BUFFER_SIZE 256
static char kbd_buffer[KBD_BUFFER_SIZE];
static uint16_t kbd_buffer_read_idx = 0;
static uint16_t kbd_buffer_write_idx = 0;

// Scancode Set 1 to ASCII mapping for a US QWERTY keyboard layout.
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
 * @brief Waits until the PS/2 controller's input buffer is empty.
 */
static void kbd_wait_for_input() {
    while(inb(KBD_STATUS_PORT) & KBD_INPUT_BUFFER_FULL);
}

/**
 * @brief Waits until the PS/2 controller's output buffer is full.
 */
static void kbd_wait_for_output() {
    while(!(inb(KBD_STATUS_PORT) & KBD_OUTPUT_BUFFER_FULL));
}


static void keyboard_handler(registers_t *regs) {
    (void)regs;
    
    // Read the scancode from the keyboard data port.
    uint8_t scancode = inb(KBD_DATA_PORT);

    // Send an EOI to the PIC immediately after reading the scancode.
    // The IRQ for the keyboard is IRQ1, which is remapped to interrupt 33.
    pic_send_eoi(1);

    // Only process "make" codes (key press) and ignore "break" codes (key release).
    // Scancodes for key presses are < 128.
    if (scancode < 128) {
        char c = kbd_us[scancode];
        if (c != 0) {
            // Handle backspace separately.
            kbd_buffer[kbd_buffer_write_idx] = c;
            kbd_buffer_write_idx = (kbd_buffer_write_idx + 1) % KBD_BUFFER_SIZE;
        }
    }
}

void keyboard_init() {
    fb_print_string("Keyboard init: starting.\n", 0x00FFFF00);
    fb_swap_buffers();

    // 1. Disable the first PS/2 port to ensure a clean state.
    kbd_wait_for_input();
    outb(KBD_CMD_PORT, KBD_DISABLE_FIRST_PORT);

    // 2. Read and modify the PS/2 controller's configuration byte.
    kbd_wait_for_input();
    outb(KBD_CMD_PORT, KBD_READ_CONFIG);
    kbd_wait_for_output();
    uint8_t config = inb(KBD_DATA_PORT);
    
    // Enable the first port interrupt.
    config |= KBD_FIRST_PORT_INTERRUPT;

    // 3. Write the new configuration byte back.
    kbd_wait_for_input();
    outb(KBD_CMD_PORT, KBD_WRITE_CONFIG);
    kbd_wait_for_input();
    outb(KBD_DATA_PORT, config);
    
    // 4. Flush the output buffer, just in case.
    while(inb(KBD_STATUS_PORT) & KBD_OUTPUT_BUFFER_FULL) {
        inb(KBD_DATA_PORT);
    }
    
    // 5. Send command to the controller to enable the first PS/2 port.
    kbd_wait_for_input();
    outb(KBD_CMD_PORT, KBD_ENABLE_FIRST_PORT);
    
    // 6. Send command to the keyboard device to set the typematic rate and delay.
    // The value 0x7F sets the delay to 1000ms and the repeat rate to 2 Hz (slowest).
    kbd_wait_for_input();
    outb(KBD_DATA_PORT, KBD_SET_TYPEMATIC_RATE_DELAY);
    kbd_wait_for_output();
    inb(KBD_DATA_PORT); // Read and discard the ACK byte.
    
    kbd_wait_for_input();
    outb(KBD_DATA_PORT, 0x5F); // Set a slow repeat rate and a long delay.
    kbd_wait_for_output();
    inb(KBD_DATA_PORT); // Read and discard the ACK byte.

    // 7. Send command to the keyboard device to enable scanning.
    kbd_wait_for_input();
    outb(KBD_DATA_PORT, KBD_ENABLE_SCANNING);
    kbd_wait_for_output(); // Wait for ACK from keyboard
    inb(KBD_DATA_PORT); // Read and discard the ACK byte.

    fb_print_string("Keyboard init: Enable scanning command sent.\n", 0x00FFFF00);
    fb_swap_buffers();

    // 8. Register our interrupt handler for IRQ 1
    register_interrupt_handler(33, keyboard_handler);

    // 9. Unmask IRQ1 on the master PIC so we can receive interrupts
    uint8_t current_mask = inb(PIC1_DATA);
    outb(PIC1_DATA, current_mask & 0b11111101);

    fb_print_string("Keyboard Initialized and Unmasked.\n", 0x00FFFF00);
    fb_swap_buffers();
}

char keyboard_getchar() {
    if (kbd_buffer_read_idx == kbd_buffer_write_idx) {
        return 0;
    }
    
    char c = kbd_buffer[kbd_buffer_read_idx];
    kbd_buffer_read_idx = (kbd_buffer_read_idx + 1) % KBD_BUFFER_SIZE;
    return c;
}

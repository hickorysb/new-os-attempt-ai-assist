#pragma once

#include <cstdint>

/**
 * @brief Reads a byte from the specified I/O port.
 * @param port The I/O port to read from.
 * @return The byte read from the port.
 */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    // The "in" instruction reads a byte from a port.
    // "=a"(ret) means the result goes into the EAX register (aliased as 'a').
    // "Nd"(port) means the port number goes into the EDX register (aliased as 'd').
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * @brief Writes a byte to the specified I/O port.
 * @param port The I/O port to write to.
 * @param val The byte value to write.
 */
static inline void outb(uint16_t port, uint8_t val) {
    // The "out" instruction writes a byte to a port.
    // "a"(val) means the value to write is in the EAX register.
    // "Nd"(port) means the port number is in the EDX register.
    asm volatile ("outb %0, %1" :: "a"(val), "Nd"(port));
}

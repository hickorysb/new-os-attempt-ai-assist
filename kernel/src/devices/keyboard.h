#pragma once

/**
 * @brief Initializes the keyboard driver by registering its interrupt handler.
 */
void keyboard_init();

/**
 * @brief Gets a character from the keyboard input buffer.
 * @return The character read, or 0 if the buffer is empty.
 */
char keyboard_getchar();

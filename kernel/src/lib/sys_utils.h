#pragma once

#include <cstdint>
#include <cstddef>
#include <limine.h>

// Simple busy-wait delay function
void delay(long long nanoseconds);

// Function to convert an integer to a string
void itoa(int n, char s[]);

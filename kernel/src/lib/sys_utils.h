#pragma once

#include <cstdint>
#include <cstddef>
#include <limine.h>

// Simple busy-wait delay function
void delay(long long nanoseconds);

// Function to convert a signed integer to a string
void itoa(int n, char s[]);

// Function to convert a 64-bit unsigned integer to a string
void u64_to_str(uint64_t n, char s[]);

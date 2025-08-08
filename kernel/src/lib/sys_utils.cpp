#include <cstdint>
#include <cstddef>
#include <limine.h>
#include "sys_utils.h"
#include "../font.h"
#include "memory/memory.h"

// Simple busy-wait delay function
void delay(long long nanoseconds) {
    volatile long long counter = 0;
    while (counter < nanoseconds) {
        counter = counter + 1;
    }
}

// Function to convert a signed integer to a string
void itoa(int n, char s[]) {
    int i, sign;
    if ((sign = n) < 0) {
        n = -n;
    }
    i = 0;
    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);
    if (sign < 0) {
        s[i++] = '-';
    }
    s[i] = '\0';
    // Reverse the string
    int j = 0;
    for (i--; j < i; i--, j++) {
        char temp = s[j];
        s[j] = s[i];
        s[i] = temp;
    }
}

// Function to convert a 64-bit unsigned integer to a string
void u64_to_str(uint64_t n, char* s) {
    if (n == 0) {
        s[0] = '0';
        s[1] = '\0';
        return;
    }

    char buf[21] = {0};
    int i = 20;

    for(; n > 0; n /= 10) {
        buf[--i] = "0123456789"[n % 10];
    }

    int len = 20 - i;
    memcpy(s, &buf[i], len);
    s[len] = '\0';
}

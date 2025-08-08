#include <cstdint>
#include <cstddef>
#include <limine.h>
#include "sys_utils.h"
#include "../font.h"

// Simple busy-wait delay function
void delay(long long nanoseconds) {
    volatile long long counter = 0;
    while (counter < nanoseconds) {
        counter = counter + 1;
    }
}

// Function to convert an integer to a string
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

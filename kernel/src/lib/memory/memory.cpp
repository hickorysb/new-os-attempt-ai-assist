#include "memory.h"

// These are the standard C library memory functions. The compiler may
// generate calls to these functions automatically, so it is important
// that they are implemented correctly with C linkage.

extern "C" {

// Copies `n` bytes from `src` to `dest`.
void *memcpy(void *__restrict dest, const void *__restrict src, std::size_t n) {
    auto *pdest = static_cast<std::uint8_t *__restrict>(dest);
    const auto *psrc = static_cast<const std::uint8_t *__restrict>(src);
    for (std::size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }
    return dest;
}

// Fills the first `n` bytes of the memory area pointed to by `s`
// with the constant byte `c`.
void *memset(void *s, int c, std::size_t n) {
    auto *p = static_cast<std::uint8_t *>(s);
    for (std::size_t i = 0; i < n; i++) {
        p[i] = static_cast<uint8_t>(c);
    }
    return s;
}

// Copies `n` bytes from `src` to `dest`. The memory areas may overlap.
void *memmove(void *dest, const void *src, std::size_t n) {
    auto *pdest = static_cast<std::uint8_t *>(dest);
    const auto *psrc = static_cast<const std::uint8_t *>(src);
    if (src > dest) {
        for (std::size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if (src < dest) {
        for (std::size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }
    return dest;
}

// Compares the first `n` bytes of `s1` and `s2`.
int memcmp(const void *s1, const void *s2, std::size_t n) {
    const auto *p1 = static_cast<const std::uint8_t *>(s1);
    const auto *p2 = static_cast<const std::uint8_t *>(s2);
    for (std::size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }
    return 0;
}

} // extern "C"

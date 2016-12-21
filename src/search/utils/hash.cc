#include "hash.h"

namespace utils {
unsigned int hash_unsigned_int_sequence(
    const unsigned int *key, unsigned int length) {
    // Set up the internal state.
    unsigned int a = 0xdeadbeef + (length << 2);
    unsigned int b = a;
    unsigned int c = a;

    // Handle most of the key.
    while (length > 3) {
        a += key[0];
        b += key[1];
        c += key[2];
        mix(a, b, c);
        length -= 3;
        key += 3;
    }

    // Handle the last 3 unsigned ints.
    if (length == 3)
        c += key[2];
    if (length >= 2)
        b += key[1];
    if (length >= 1) {
        a += key[0];
        final_mix(a, b, c);
    }

    return c;
}
}

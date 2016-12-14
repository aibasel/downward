#include "hash.h"

namespace utils {
/*
  The functions rotate(), mix(), final_mix() and
  hash_unsigned_int_sequence() have been taken and adapted from
  lookup3.c, by Bob Jenkins, May 2006, Public Domain.
  (http://www.burtleburtle.net/bob/c/lookup3.c)

  hash_unsigned_int_sequence() corresponds to the hashword() function
  in lookup3.c.
*/

/*
  Circular rotation.
*/
static inline unsigned int rotate(unsigned int value, unsigned int offset) {
    return (value << offset) | (value >> (32 - offset));
}

/*
  mix 3 32-bit values reversibly.

  Any information in (a, b, c) before mix() is still in (a, b, c) after
  mix().
*/
static inline void mix(unsigned int &a, unsigned int &b, unsigned int &c) {
    a -= c;
    a ^= rotate(c, 4);
    c += b;
    b -= a;
    b ^= rotate(a, 6);
    a += c;
    c -= b;
    c ^= rotate(b, 8);
    b += a;
    a -= c;
    a ^= rotate(c, 16);
    c += b;
    b -= a;
    b ^= rotate(a, 19);
    a += c;
    c -= b;
    c ^= rotate(b, 4);
    b += a;
}

/*
  final mixing of 3 32-bit values (a, b, c) into c.

  Pairs of (a,b,c) values differing in only a few bits will usually
  produce values of c that look totally different.
*/
static inline void final_mix(unsigned int &a, unsigned int &b, unsigned int &c) {
    c ^= b;
    c -= rotate(b, 14);
    a ^= c;
    a -= rotate(c, 11);
    b ^= a;
    b -= rotate(a, 25);
    c ^= b;
    c -= rotate(b, 16);
    a ^= c;
    a -= rotate(c, 4);
    b ^= a;
    b -= rotate(a, 14);
    c ^= b;
    c -= rotate(b, 24);
}

unsigned int hash_unsigned_int_sequence(
    const unsigned int *key, unsigned int length, unsigned int initial_value) {
    unsigned int a, b, c;

    // Set up the internal state.
    a = b = c = 0xdeadbeef + (length << 2) + initial_value;

    // Handle most of the key.
    while (length > 3) {
        a += key[0];
        b += key[1];
        c += key[2];
        mix(a, b, c);
        length -= 3;
        key += 3;
    }

    // Handle the last 3 unsigned ints. All case statements fall through.
    switch (length) {
    case 3:
        c += key[2];
    case 2:
        b += key[1];
    case 1:
        a += key[0];
        final_mix(a, b, c);
    // case 0: nothing left to add.
    case 0:
        break;
    }

    return c;
}
}

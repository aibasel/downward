#include "hash.h"

namespace utils {
/*
  Code for rot(), mix(), final() and hash_unsigned_int_sequence() taken
  from lookup3.c, by Bob Jenkins, May 2006, Public Domain.
  (http://www.burtleburtle.net/bob/c/lookup3.c)

  hash_unsigned_int_sequence() corresponds to the hashword() function
  in lookup3.c.
*/

/*
  Circular rotation.
*/
static inline unsigned int rot(unsigned int x, unsigned int k) {
    return (x << k) | (x >> (32 - k));
}

/*
  mix 3 32-bit values reversibly.

  Any information in (a,b,c) before mix() is still in (a,b,c) after
  mix().
*/
static inline void mix(unsigned int &a, unsigned int &b, unsigned int &c) {
    a -= c;
    a ^= rot(c, 4);
    c += b;
    b -= a;
    b ^= rot(a, 6);
    a += c;
    c -= b;
    c ^= rot(b, 8);
    b += a;
    a -= c;
    a ^= rot(c, 16);
    c += b;
    b -= a;
    b ^= rot(a, 19);
    a += c;
    c -= b;
    c ^= rot(b, 4);
    b += a;
}

/*
  final mixing of 3 32-bit values (a,b,c) into c.

  Pairs of (a,b,c) values differing in only a few bits will usually
  produce values of c that look totally different.
*/
static inline void final (unsigned int &a, unsigned int &b, unsigned int &c) {
    c ^= b;
    c -= rot(b, 14);
    a ^= c;
    a -= rot(c, 11);
    b ^= a;
    b -= rot(a, 25);
    c ^= b;
    c -= rot(b, 16);
    a ^= c;
    a -= rot(c, 4);
    b ^= a;
    b -= rot(a, 14);
    c ^= b;
    c -= rot(b, 24);
}

unsigned int hash_unsigned_int_sequence(
    const unsigned int *k, unsigned int length, unsigned int initval) {
    unsigned int a, b, c;

    // Set up the internal state.
    a = b = c = 0xdeadbeef + (length << 2) + initval;

    // Handle most of the key.
    while (length > 3) {
        a += k[0];
        b += k[1];
        c += k[2];
        mix(a, b, c);
        length -= 3;
        k += 3;
    }

    // Handle the last 3 unsigned ints. All case statements fall through.
    switch (length) {
    case 3:
        c += k[2];
    case 2:
        b += k[1];
    case 1:
        a += k[0];
        final (a, b, c);
    // case 0: nothing left to add.
    case 0:
        break;
    }

    return c;
}
}

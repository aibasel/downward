#ifndef RNG_H
#define RNG_H

class RandomNumberGenerator {
    static const int N = 624;
    unsigned int mt[N];
    int mti;
public:
    RandomNumberGenerator();         // seed with time-dependent value
    RandomNumberGenerator(int seed); // seed with int; see comments for seed()
    RandomNumberGenerator(unsigned int *array, int count); // seed with array
    RandomNumberGenerator(const RandomNumberGenerator &copy);
    RandomNumberGenerator &operator=(const RandomNumberGenerator &copy);

    void seed(int s);
    void seed(unsigned int *array, int len);

    unsigned int next32();      // random integer in [0..2^32-1]
    int next31();               // random integer in [0..2^31-1]
    double next_half_open();    // random float in [0..1), 2^53 possible values
    double next_closed();       // random float in [0..1], 2^53 possible values
    double next_open();         // random float in (0..1), 2^53 possible values
    int next(int bound);        // random integer in [0..bound), bound < 2^31
    int operator()(int bound) { // same as next()
        return next(bound);
    }
    double next();              // same as next_half_open()
    double operator()() {       // same as next_half_open()
        return next_half_open();
    }
};

/*
  TODO: Add a static assertion that guarantees that ints are 32 bit.
        In cases where they are not, need to adapt the code.
 */

/*
  Notes on seeding

  1. Seeding with an integer
  To avoid different seeds mapping to the same sequence, follow one of
  the following two conventions:
  a) Only use seeds in 0..2^31-1     (preferred)
  b) Only use seeds in -2^30..2^30-1 (2-complement machines only)

  2. Seeding with an array (die-hard seed method)
  The length of the array, len, can be arbitrarily high, but for lengths greater
  than N, collisions are common. If the seed is of high quality, using more than
  N values does not make sense.
*/

#endif

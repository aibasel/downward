#ifndef RNG_H
#define RNG_H

#include <algorithm>
#include <random>
#include <vector>

class RandomNumberGenerator {
    // Mersenne Twister random number generator.
    std::mt19937 rng;
public:
    RandomNumberGenerator();          // seed with time-dependent value
    RandomNumberGenerator(int seed_); // seed with integer
    RandomNumberGenerator(const RandomNumberGenerator &) = delete;
    RandomNumberGenerator &operator=(const RandomNumberGenerator &) = delete;

    void seed(int seed);

    unsigned int next32();      // random integer in [0..2^32-1]
    int next31();               // random integer in [0..2^31-1]
    double next_half_open();    // random float in [0..1), 2^53 possible values
    double next_closed();       // random float in [0..1], 2^53 possible values
    double next_open();         // random float in (0..1), 2^53 possible values
    int next(int bound);        // random integer in [0..bound), bound < 2^31
    int operator()(int bound) { // same as next()
        return next(bound);
    }
    double operator()() {       // same as next_half_open()
        return next_half_open();
    }
    template <class T>
    void shuffle(std::vector<T> &vec) {
        std::shuffle(vec.begin(), vec.end(), rng);
    }
};

#endif

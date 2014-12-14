#ifndef RNG_H
#define RNG_H

#include <algorithm>
#include <random>
#include <vector>

class RandomNumberGenerator {
    // Mersenne Twister random number generator.
    std::mt19937 rng;
public:
    RandomNumberGenerator();                   // seed with time-dependent value
    explicit RandomNumberGenerator(int seed_); // seed with integer
    RandomNumberGenerator(const RandomNumberGenerator &) = delete;
    RandomNumberGenerator &operator=(const RandomNumberGenerator &) = delete;

    void seed(int seed);

    // random integer in [0..2^32-1]
    unsigned int next_uint() {
        return rng();
    }

    double operator()();        // random double in [0..1), 2^53 possible values
    int operator()(int bound);  // random integer in [0..bound), bound < 2^31

    template <class T>
    void shuffle(std::vector<T> &vec) {
        std::shuffle(vec.begin(), vec.end(), rng);
    }
};

#endif

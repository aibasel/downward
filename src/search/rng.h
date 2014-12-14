#ifndef RNG_H
#define RNG_H

#include <algorithm>
#include <cassert>
#include <random>
#include <vector>

class RandomNumberGenerator {
    // Mersenne Twister random number generator.
    std::mt19937 rng;

public:
    // Seed with time-dependent value.
    RandomNumberGenerator();

    // Seed with integer.
    explicit RandomNumberGenerator(int seed_);

    RandomNumberGenerator(const RandomNumberGenerator &) = delete;
    RandomNumberGenerator &operator=(const RandomNumberGenerator &) = delete;

    void seed(int seed);

    // Return random double in [0..1).
    double operator()() {
        std::uniform_real_distribution<double> distribution(0.0, 1.0);
        return distribution(rng);
    }

    // Return random integer in [0..bound).
    int operator()(int bound) {
        assert(bound > 0);
        std::uniform_int_distribution<int> distribution(0, bound - 1);
        return distribution(rng);
    }

    template <class T>
    void shuffle(std::vector<T> &vec) {
        std::shuffle(vec.begin(), vec.end(), rng);
    }
};

#endif

#ifndef INLINED_RNG_H
#define INLINED_RNG_H

#include <cassert>
#include <random>

class InlinedRandomNumberGenerator {
    std::mt19937 rng;
public:
    explicit InlinedRandomNumberGenerator(int seed) {
        rng.seed(seed);
    }

    double operator()() {
        std::uniform_real_distribution<double> distribution(0.0, 1.0);
        return distribution(rng);
    }

    int operator()(int bound) {
        assert(bound > 0);
        std::uniform_int_distribution<int> distribution(0, bound - 1);
        return distribution(rng);
    }
};

#endif

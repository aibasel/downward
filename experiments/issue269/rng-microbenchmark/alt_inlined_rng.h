#ifndef ALT_INLINED_RNG_H
#define ALT_INLINED_RNG_H

#include <cassert>
#include <random>

class AltInlinedRandomNumberGenerator {
    std::mt19937 rng;
    std::uniform_real_distribution<double> double_distribution {
        0.0, 1.0
    };
public:
    explicit AltInlinedRandomNumberGenerator(int seed) {
        rng.seed(seed);
    }

    double operator()() {
        return double_distribution(rng);
    }

    int operator()(int bound) {
        assert(bound > 0);
        std::uniform_int_distribution<int> distribution(0, bound - 1);
        return distribution(rng);
    }
};

#endif

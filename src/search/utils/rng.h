#ifndef UTILS_RNG_H
#define UTILS_RNG_H

#include <algorithm>
#include <cassert>
#include <memory>
#include <random>
#include <vector>

namespace options {
class OptionParser;
class Options;
}

namespace utils {
class RandomNumberGenerator {
    // Mersenne Twister random number generator.
    std::mt19937 rng_;

public:
    RandomNumberGenerator(); // Seed with a value depending on time and process ID.
    explicit RandomNumberGenerator(int seed);
    RandomNumberGenerator(const RandomNumberGenerator &) = delete;
    RandomNumberGenerator &operator=(const RandomNumberGenerator &) = delete;
    ~RandomNumberGenerator();

    void seed(int seed);

    // Return random double in [0..1).
    double operator()() {
        std::uniform_real_distribution<double> distribution(0.0, 1.0);
        return distribution(rng_);
    }

    // Return random integer in [0..bound).
    int operator()(int bound) {
        assert(bound > 0);
        std::uniform_int_distribution<int> distribution(0, bound - 1);
        return distribution(rng_);
    }

    template<typename T>
    typename std::vector<T>::const_iterator choose(const std::vector<T> &vec) {
        return vec.begin() + operator()(vec.size());
    }

    template<typename T>
    typename std::vector<T>::iterator choose(std::vector<T> &vec) {
        return vec.begin() + operator()(vec.size());
    }

    template<typename T>
    void shuffle(std::vector<T> &vec) {
        std::shuffle(vec.begin(), vec.end(), rng_);
    }

    // Add random_seed option to parser.
    static void add_rng_options(options::OptionParser &parser);

    /*
      The parameter options has to contain the key "random_seed".
      If random_seed = -1, return the rng object from rng(). Otherwise,
      create and return a pointer to a new rng object with the given seed.
    */
    static std::shared_ptr<RandomNumberGenerator> parse_rng_from_options(
        const options::Options &options);

    /*
      Return a shared pointer to a static rng object of this method, which is
      only created on demand. Replaced the old "g_rng" object.
    */
    static std::shared_ptr<RandomNumberGenerator> rng();
};
}

#endif

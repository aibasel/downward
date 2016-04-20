#ifndef UTILS_RNG_OPTIONS_H
#define UTILS_RNG_OPTIONS_H

#include <memory>

namespace options {
class OptionParser;
class Options;
}

namespace utlis {
class RandomNumberGenerator;

// Add random_seed option to parser.
extern void add_rng_options(options::OptionParser &parser);

/*
  The parameter options has to contain the key "random_seed".
  If random_seed = -1, return the rng object from rng(). Otherwise,
  create and return a pointer to a new rng object with the given seed.
*/
extern std::shared_ptr<RandomNumberGenerator> parse_rng_from_options(
    const options::Options &options);
}

#endif

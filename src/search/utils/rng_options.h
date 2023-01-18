#ifndef UTILS_RNG_OPTIONS_H
#define UTILS_RNG_OPTIONS_H

#include <memory>

namespace plugins {
class Feature;
class Options;
}

namespace utils {
class RandomNumberGenerator;

// Add random_seed option to parser.
extern void add_rng_options(plugins::Feature &feature);

/*
  Return an RNG based on the given options, which can either be the global
  RNG or a local one with a user-specified seed. Only use this together with
  "add_rng_options()".
*/
extern std::shared_ptr<RandomNumberGenerator> parse_rng_from_options(
    const plugins::Options &options);
}

#endif

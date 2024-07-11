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
extern void add_rng_options_to_feature(plugins::Feature &feature);
extern std::tuple<int> get_rng_arguments_from_options(
    const plugins::Options &opts);

/*
  Return an RNG for the given seed, which can either be the global
  RNG or a local one with a user-specified seed. Only use this together
  with "add_rng_options_to_feature()".
*/
extern std::shared_ptr<RandomNumberGenerator> get_rng(int seed);
}

#endif

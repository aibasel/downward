#ifndef POTENTIALS_UTIL_H
#define POTENTIALS_UTIL_H

#include <memory>
#include <string>
#include <vector>

class State;

namespace plugins {
class Feature;
}

namespace utils {
class RandomNumberGenerator;
}

namespace potentials {
class PotentialOptimizer;

std::vector<State> sample_without_dead_end_detection(
    PotentialOptimizer &optimizer,
    int num_samples,
    utils::RandomNumberGenerator &rng);

std::string get_admissible_potentials_reference();
void add_admissible_potentials_options_to_feature(plugins::Feature &feature, const std::string &description);
}

#endif

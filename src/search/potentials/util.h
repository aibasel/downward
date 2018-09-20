#ifndef POTENTIALS_UTIL_H
#define POTENTIALS_UTIL_H

#include <memory>
#include <string>
#include <vector>

class State;

namespace options {
class OptionParser;
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
void prepare_parser_for_admissible_potentials(options::OptionParser &parser);
}

#endif

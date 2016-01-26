#ifndef POTENTIALS_UTIL_H
#define POTENTIALS_UTIL_H

#include <string>
#include <vector>

class State;

namespace options {
class OptionParser;
}

namespace potentials {
class PotentialOptimizer;

std::vector<State> sample_without_dead_end_detection(
    PotentialOptimizer &optimizer, int num_samples);

std::string get_admissible_potentials_reference();
void prepare_parser_for_admissible_potentials(options::OptionParser &parser);
}

#endif

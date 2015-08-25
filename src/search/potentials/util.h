#ifndef POTENTIALS_UTIL_H
#define POTENTIALS_UTIL_H

#include <vector>

class State;
class OptionParser;

namespace potentials {
class PotentialOptimizer;

std::vector<State> sample_without_dead_end_detection(
    PotentialOptimizer &optimizer, int num_samples);

void prepare_parser_for_admissible_potentials(OptionParser &parser);
}

#endif

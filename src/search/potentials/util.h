#ifndef POTENTIALS_UTIL_H
#define POTENTIALS_UTIL_H

#include <vector>

class State;

namespace potentials {
class PotentialOptimizer;

std::vector<State> sample_without_dead_end_detection(
    PotentialOptimizer &optimizer, int num_samples);

void optimize_for_samples(PotentialOptimizer &optimizer, int num_samples);
}

#endif

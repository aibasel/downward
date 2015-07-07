#ifndef POTENTIALS_SINGLE_POTENTIAL_HEURISTICS_H
#define POTENTIALS_SINGLE_POTENTIAL_HEURISTICS_H

#include <vector>

class GlobalState;
class StateRegistry;

namespace potentials {
class PotentialOptimizer;

std::vector<GlobalState> sample_without_dead_end_detection(
    PotentialOptimizer &optimizer, StateRegistry &sample_registry, int num_samples);

void optimize_for_samples(PotentialOptimizer &optimizer, int num_samples);
}

#endif

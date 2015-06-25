#ifndef SAMPLING_H
#define SAMPLING_H

#include <vector>

class GlobalState;
class Heuristic;
class StateRegistry;

std::vector<GlobalState> sample_states_with_random_walks(
    StateRegistry &sample_registry,
    int num_samples,
    Heuristic &heuristic);

#endif

#include "util.h"

#include "potential_function.h"
#include "potential_optimizer.h"

#include "../globals.h"
#include "../sampling.h"
#include "../state_registry.h"

using namespace std;


namespace potentials {

void filter_dead_ends(PotentialOptimizer &optimizer, vector<GlobalState> &samples) {
    assert(!optimizer.potentials_are_bounded());
    vector<GlobalState> non_dead_end_samples;
    for (const GlobalState &sample : samples) {
        if (optimizer.optimize_for_state(sample))
            non_dead_end_samples.push_back(sample);
    }
    swap(samples, non_dead_end_samples);
}

vector<GlobalState> sample_without_dead_end_detection(
    PotentialOptimizer &optimizer, StateRegistry &sample_registry, int num_samples) {
    optimizer.optimize_for_state(g_initial_state());
    int init_h = optimizer.get_potential_function()->get_value(g_initial_state());
    return sample_states_with_random_walks(
        sample_registry, num_samples, init_h, [](const GlobalState &) {
            // Currently, our potential functions can't detect dead ends.
            return false;
            });
}

void optimize_for_samples(PotentialOptimizer &optimizer, int num_samples) {
    StateRegistry sample_registry;
    vector<GlobalState> samples = sample_without_dead_end_detection(
        optimizer, sample_registry, num_samples);
    if (!optimizer.potentials_are_bounded()) {
        filter_dead_ends(optimizer, samples);
    }
    optimizer.optimize_for_samples(samples);
}

}

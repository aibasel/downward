#include "util.h"

#include "potential_function.h"
#include "potential_optimizer.h"

#include "../countdown_timer.h"
#include "../sampling.h"
#include "../successor_generator.h"

#include <limits>

using namespace std;


namespace potentials {
void filter_dead_ends(PotentialOptimizer &optimizer, vector<State> &samples) {
    assert(!optimizer.potentials_are_bounded());
    vector<State> non_dead_end_samples;
    for (const State &sample : samples) {
        if (optimizer.optimize_for_state(sample))
            non_dead_end_samples.push_back(sample);
    }
    swap(samples, non_dead_end_samples);
}

vector<State> sample_without_dead_end_detection(
    PotentialOptimizer &optimizer, int num_samples) {
    const shared_ptr<AbstractTask> task = optimizer.get_task();
    const TaskProxy task_proxy(*task);
    State initial_state = task_proxy.get_initial_state();
    optimizer.optimize_for_state(initial_state);
    SuccessorGenerator successor_generator(task);
    int init_h = optimizer.get_potential_function()->get_value(initial_state);
    CountdownTimer timer(numeric_limits<double>::infinity());
    return sample_states_with_random_walks(
               task_proxy, successor_generator, num_samples, init_h,
               get_average_operator_cost(task_proxy),
               [] (const State &) {
                   // Currently, our potential functions can't detect dead ends.
                   return false;
               },
               timer);
}

void optimize_for_samples(PotentialOptimizer &optimizer, int num_samples) {
    vector<State> samples = sample_without_dead_end_detection(
        optimizer, num_samples);
    if (!optimizer.potentials_are_bounded()) {
        filter_dead_ends(optimizer, samples);
    }
    optimizer.optimize_for_samples(samples);
}
}

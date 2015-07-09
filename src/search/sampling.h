#ifndef SAMPLING_H
#define SAMPLING_H

#include "globals.h"
#include "rng.h"
#include "task_proxy.h"
#include "task_tools.h"

#include <vector>

/* TODO: issue529 lets the iPDB random walks use the task interface. Once that
   issue is merged we can use the State class in the potentials code as well
   and avoid the code duplication with iPDB. */
template<class Callback>
std::vector<State> sample_states_with_random_walks(
    TaskProxy task_proxy,
    int num_samples,
    int init_h,
    const Callback &is_dead_end) {
    std::vector<State> samples;

    double average_operator_cost = 0;
    for (OperatorProxy op : task_proxy.get_operators())
        average_operator_cost += op.get_cost();
    average_operator_cost /= task_proxy.get_operators().size();

    const State &initial_state = task_proxy.get_initial_state();

    int n;
    if (init_h == 0) {
        n = 10;
    } else {
        // Convert heuristic value into an approximate number of actions
        // (does nothing on unit-cost problems).
        // average_operator_cost cannot equal 0, as in this case, all operators
        // must have costs of 0 and in this case the if-clause triggers.
        int solution_steps_estimate = int((init_h / average_operator_cost) + 0.5);
        n = 4 * solution_steps_estimate;
    }
    double p = 0.5;
    // The expected walk length is np = 2 * estimated number of solution steps.
    // (We multiply by 2 because the heuristic is underestimating.)

    samples.reserve(num_samples);
    for (int i = 0; i < num_samples; ++i) {
        // calculate length of random walk accoring to a binomial distribution
        int length = 0;
        for (int j = 0; j < n; ++j) {
            double random = g_rng(); // [0..1)
            if (random < p)
                ++length;
        }

        // random walk of length length
        State current_state(initial_state);
        for (int j = 0; j < length; ++j) {
            // TODO: Use successor generator.
            std::vector<OperatorProxy> applicable_ops;
            for (OperatorProxy op : task_proxy.get_operators()) {
                if (is_applicable(op, current_state)) {
                    applicable_ops.push_back(op);
                }
            }
            // if there are no applicable operators --> do not walk further
            if (applicable_ops.empty()) {
                break;
            } else {
                const OperatorProxy random_op = *g_rng.choose(applicable_ops);
                assert(is_applicable(random_op, current_state));
                current_state = current_state.get_successor(random_op);
                // if current state is a dead end, then restart with initial state
                if (is_dead_end(current_state))
                    current_state = State(initial_state);
            }
        }
        // last state of the random walk is used as sample
        samples.push_back(current_state);
    }
    return samples;
}

#endif

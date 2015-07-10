#ifndef SAMPLING_H
#define SAMPLING_H

#include "countdown_timer.h"
#include "globals.h"
#include "rng.h"
#include "task_proxy.h"
#include "task_tools.h"

#include <exception>
#include <vector>


struct SamplingTimeout : public std::exception {};

double get_average_operator_cost(TaskProxy task_proxy);

template<class Callback>
std::vector<State> sample_states_with_random_walks(
    TaskProxy task_proxy,
    int num_samples,
    int init_h,
    int average_operator_cost,
    const Callback &is_dead_end,
    const CountdownTimer &timer) {
    std::vector<State> samples;

    const State &initial_state = task_proxy.get_initial_state();

    int n;
    if (init_h == 0) {
        n = 10;
    } else {
        /*
          Convert heuristic value into an approximate number of actions
          (does nothing on unit-cost problems).
          average_operator_cost cannot equal 0, as in this case, all operators
          must have costs of 0 and in this case the if-clause triggers.
        */
        int solution_steps_estimate = int((init_h / average_operator_cost) + 0.5);
        n = 4 * solution_steps_estimate;
    }
    double p = 0.5;
    /* The expected walk length is np = 2 * estimated number of solution steps.
       (We multiply by 2 because the heuristic is underestimating.) */

    samples.reserve(num_samples);
    for (int i = 0; i < num_samples; ++i) {
        if (timer.is_expired())
            throw SamplingTimeout();

        // Calculate length of random walk according to a binomial distribution.
        int length = 0;
        for (int j = 0; j < n; ++j) {
            double random = g_rng(); // [0..1)
            if (random < p)
                ++length;
        }

        // Sample one state with a random walk of length length.
        State current_state(initial_state);
        for (int j = 0; j < length; ++j) {
            /* TODO we want to use the successor generator here but it only
               handles GlobalState objects */
            std::vector<OperatorProxy> applicable_ops;
            for (OperatorProxy op : task_proxy.get_operators()) {
                if (is_applicable(op, current_state)) {
                    applicable_ops.push_back(op);
                }
            }
            // If there are no applicable operators, do not walk further.
            if (applicable_ops.empty()) {
                break;
            } else {
                const OperatorProxy &random_op = *g_rng.choose(applicable_ops);
                assert(is_applicable(random_op, current_state));
                current_state = current_state.get_successor(random_op);
                /* If current state is a dead end, then restart the random walk
                   with the initial state. */
                if (is_dead_end(current_state))
                    current_state = State(initial_state);
            }
        }
        // The last state of the random walk is used as a sample.
        samples.push_back(current_state);
    }
    return samples;
}

#endif

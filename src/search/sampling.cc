#include "sampling.h"

#include "countdown_timer.h"
#include "globals.h"
#include "rng.h"
#include "successor_generator.h"
#include "task_proxy.h"
#include "task_tools.h"

using namespace std;


vector<State> sample_states_with_random_walks(
    TaskProxy task_proxy,
    SuccessorGenerator &successor_generator,
    int num_samples,
    int init_h,
    double average_operator_cost,
    function<bool (State)> is_dead_end,
    const CountdownTimer *timer) {
    vector<State> samples;

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
        assert(average_operator_cost != 0);
        int solution_steps_estimate = int((init_h / average_operator_cost) + 0.5);
        n = 4 * solution_steps_estimate;
    }
    double p = 0.5;
    /* The expected walk length is np = 2 * estimated number of solution steps.
       (We multiply by 2 because the heuristic is underestimating.) */

    samples.reserve(num_samples);
    for (int i = 0; i < num_samples; ++i) {
        if (timer && timer->is_expired())
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
        vector<OperatorProxy> applicable_ops;
        for (int j = 0; j < length; ++j) {
            applicable_ops.clear();
            successor_generator.generate_applicable_ops(current_state,
                                                        applicable_ops);
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

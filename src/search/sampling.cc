#include "sampling.h"

#include "evaluation_context.h"
#include "global_operator.h"
#include "globals.h"
#include "heuristic.h"
#include "rng.h"
#include "state_registry.h"
#include "successor_generator.h"
#include "timer.h"

using namespace std;


// TODO: Avoid code duplication with iPDB.
vector<GlobalState> sample_states_with_random_walks(
        StateRegistry &sample_registry,
        int num_samples,
        Heuristic &heuristic) {
    vector<GlobalState> samples;

    double average_operator_cost = 0;
    for (size_t i = 0; i < g_operators.size(); ++i)
        average_operator_cost += g_operators[i].get_cost();
    average_operator_cost /= g_operators.size();

    const GlobalState &initial_state = sample_registry.get_initial_state();
    EvaluationContext eval_context(initial_state);
    if (eval_context.is_heuristic_infinite(&heuristic)) {
        cout << "Initial state is a dead end, abort sampling." << endl;
        return samples;
    }
    int h = eval_context.get_heuristic_value(&heuristic);

    int n;
    if (h == 0) {
        n = 10;
    } else {
        // Convert heuristic value into an approximate number of actions
        // (does nothing on unit-cost problems).
        // average_operator_cost cannot equal 0, as in this case, all operators
        // must have costs of 0 and in this case the if-clause triggers.
        int solution_steps_estimate = int((h / average_operator_cost) + 0.5);
        n = 4 * solution_steps_estimate;
    }
    double p = 0.5;
    // The expected walk length is np = sampling_steps_factor * estimated number of solution steps.
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
        GlobalState current_state(initial_state);
        for (int j = 0; j < length; ++j) {
            vector<const GlobalOperator *> applicable_ops;
            g_successor_generator->generate_applicable_ops(current_state, applicable_ops);
            // if there are no applicable operators --> do not walk further
            if (applicable_ops.empty()) {
                break;
            } else {
                const GlobalOperator *random_op = *g_rng.choose(applicable_ops);
                assert(random_op->is_applicable(current_state));
                current_state = sample_registry.get_successor_state(current_state, *random_op);
                // if current state is a dead end, then restart with initial state
                EvaluationContext eval_context(current_state);
                if (eval_context.is_heuristic_infinite(&heuristic))
                    current_state = initial_state;
            }
        }
        // last state of the random walk is used as sample
        samples.push_back(current_state);
    }
    return samples;
}

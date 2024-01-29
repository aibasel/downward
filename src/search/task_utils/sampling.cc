#include "sampling.h"

#include "successor_generator.h"

#include "../task_proxy.h"

#include "../task_utils/task_properties.h"
#include "../utils/memory.h"
#include "../utils/rng.h"

using namespace std;


namespace sampling {
static State sample_state_with_random_walk(
    const OperatorsProxy &operators,
    const State &initial_state,
    const successor_generator::SuccessorGenerator &successor_generator,
    int init_h,
    double average_operator_cost,
    utils::RandomNumberGenerator &rng,
    const function<bool(State)> &is_dead_end) {
    assert(init_h != numeric_limits<int>::max());
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

    // Calculate length of random walk according to a binomial distribution.
    int length = 0;
    for (int j = 0; j < n; ++j) {
        double random = rng.random(); // [0..1)
        if (random < p)
            ++length;
    }

    // Sample one state with a random walk of length length.
    State current_state(initial_state);
    vector<OperatorID> applicable_operators;
    for (int j = 0; j < length; ++j) {
        applicable_operators.clear();
        successor_generator.generate_applicable_ops(current_state,
                                                    applicable_operators);
        // If there are no applicable operators, do not walk further.
        if (applicable_operators.empty()) {
            break;
        } else {
            OperatorID random_op_id = *rng.choose(applicable_operators);
            OperatorProxy random_op = operators[random_op_id];
            assert(task_properties::is_applicable(random_op, current_state));
            current_state = current_state.get_unregistered_successor(random_op);
            /* If current state is a dead end, then restart the random walk
               with the initial state. */
            if (is_dead_end(current_state)) {
                current_state = State(initial_state);
            }
        }
    }
    // The last state of the random walk is used as a sample.
    return current_state;
}


RandomWalkSampler::RandomWalkSampler(
    const TaskProxy &task_proxy,
    utils::RandomNumberGenerator &rng)
    : operators(task_proxy.get_operators()),
      successor_generator(utils::make_unique_ptr<successor_generator::SuccessorGenerator>(task_proxy)),
      initial_state(task_proxy.get_initial_state()),
      average_operator_costs(task_properties::get_average_operator_cost(task_proxy)),
      rng(rng) {
}

RandomWalkSampler::~RandomWalkSampler() {
}

State RandomWalkSampler::sample_state(
    int init_h, const DeadEndDetector &is_dead_end) const {
    return sample_state_with_random_walk(
        operators,
        initial_state,
        *successor_generator,
        init_h,
        average_operator_costs,
        rng,
        is_dead_end);
}
}

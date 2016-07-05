#include "cost_saturation.h"

#include "abstraction.h"
#include "cartesian_heuristic_function.h"
#include "subtask_generators.h"
#include "utils.h"

#include "../globals.h"
#include "../task_tools.h"

#include "../tasks/modified_operator_costs_task.h"

#include "../utils/countdown_timer.h"
#include "../utils/logging.h"
#include "../utils/memory.h"

#include <algorithm>
#include <cassert>
#include <string>

using namespace std;

namespace cegar {
/*
  We reserve some memory to be able to recover from out-of-memory
  situations gracefully. When the memory runs out, we stop refining and
  start the next refinement or the search. Due to memory fragmentation
  the memory used for building the abstraction (states, transitions,
  etc.) often can't be reused for things that require big continuous
  blocks of memory. It is for this reason that we require such a large
  amount of memory padding.
*/
static const int memory_padding_in_mb = 75;

CostSaturation::CostSaturation(
    vector<shared_ptr<SubtaskGenerator>> subtask_generators,
    int max_states,
    double max_time,
    bool use_general_costs,
    PickSplit pick_split)
    : subtask_generators(subtask_generators),
      max_states(max_states),
      max_time(max_time),
      use_general_costs(use_general_costs),
      pick_split(pick_split),
      num_abstractions(0),
      num_states(0) {
}

vector<CartesianHeuristicFunction> CostSaturation::generate_heuristic_functions(
    const std::shared_ptr<AbstractTask> &task) {
    utils::CountdownTimer timer(max_time);

    TaskProxy task_proxy(*task);

    verify_no_axioms(task_proxy);
    verify_no_conditional_effects(task_proxy);

    reset(task_proxy);

    State initial_state = TaskProxy(*task).get_initial_state();

    function<bool()> may_continue =
        [&] () {
            return num_states < max_states &&
                   !timer.is_expired() &&
                   utils::extra_memory_padding_is_reserved() &&
                   !state_is_dead_end(initial_state);
        };

    utils::reserve_extra_memory_padding(memory_padding_in_mb);
    for (shared_ptr<SubtaskGenerator> subtask_generator : subtask_generators) {
        SharedTasks subtasks = subtask_generator->get_subtasks(task);
        build_abstractions(subtasks, timer, may_continue);
        if (!may_continue())
            break;
    }
    if (utils::extra_memory_padding_is_reserved())
        utils::release_extra_memory_padding();
    print_statistics();
    vector<CartesianHeuristicFunction> functions;
    swap(heuristic_functions, functions);
    return functions;
}

void CostSaturation::reset(const TaskProxy &task_proxy) {
    remaining_costs.clear();
    remaining_costs.reserve(task_proxy.get_operators().size());
    for (OperatorProxy op : task_proxy.get_operators())
        remaining_costs.push_back(op.get_cost());

    assert(heuristic_functions.empty());
    num_abstractions = 0;
    num_states = 0;
}

void CostSaturation::reduce_remaining_costs(
    const vector<int> &saturated_costs) {
    assert(remaining_costs.size() == saturated_costs.size());
    for (size_t i = 0; i < remaining_costs.size(); ++i) {
        int &remaining = remaining_costs[i];
        const int &saturated = saturated_costs[i];
        assert(saturated <= remaining);
        /* Since we ignore transitions from states s with h(s)=INF, all
           saturated costs (h(s)-h(s')) are finite or -INF. */
        assert(saturated != INF);
        if (remaining == INF) {
            // INF - x = INF for finite values x.
        } else if (saturated == -INF) {
            remaining = INF;
        } else {
            remaining -= saturated;
        }
        assert(remaining >= 0);
    }
}

shared_ptr<AbstractTask> CostSaturation::get_remaining_costs_task(
    shared_ptr<AbstractTask> &parent) const {
    vector<int> costs = remaining_costs;
    return make_shared<extra_tasks::ModifiedOperatorCostsTask>(
        parent, move(costs));
}

bool CostSaturation::state_is_dead_end(const State &state) const {
    for (const CartesianHeuristicFunction &func : heuristic_functions) {
        if (func.get_value(state) == INF)
            return true;
    }
    return false;
}

void CostSaturation::build_abstractions(
    const vector<shared_ptr<AbstractTask>> &subtasks,
    const utils::CountdownTimer &timer,
    function<bool()> may_continue) {
    int rem_subtasks = subtasks.size();
    for (shared_ptr<AbstractTask> subtask : subtasks) {
        subtask = get_remaining_costs_task(subtask);

        assert(num_states < max_states);
        Abstraction abstraction(
            subtask,
            max(1, (max_states - num_states) / rem_subtasks),
            timer.get_remaining_time() / rem_subtasks,
            use_general_costs,
            pick_split);

        ++num_abstractions;
        num_states += abstraction.get_num_states();
        assert(num_states <= max_states);
        reduce_remaining_costs(abstraction.get_saturated_costs());
        int init_h = abstraction.get_h_value_of_initial_state();

        if (init_h > 0) {
            heuristic_functions.emplace_back(
                subtask,
                abstraction.extract_refinement_hierarchy());
        }
        if (!may_continue())
            break;

        --rem_subtasks;
    }
}

void CostSaturation::print_statistics() const {
    g_log << "Done initializing additive Cartesian heuristic" << endl;
    cout << "Cartesian abstractions built: " << num_abstractions << endl;
    cout << "Cartesian heuristic functions stored: "
         << heuristic_functions.size() << endl;
    cout << "Cartesian states: " << num_states << endl;
    cout << endl;
}
}

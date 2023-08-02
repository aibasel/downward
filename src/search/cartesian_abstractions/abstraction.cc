#include "abstraction.h"

#include "abstract_state.h"
#include "refinement_hierarchy.h"
#include "transition.h"
#include "transition_system.h"
#include "utils.h"

#include "../task_utils/task_properties.h"
#include "../utils/logging.h"
#include "../utils/math.h"
#include "../utils/memory.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <unordered_map>

using namespace std;

namespace cartesian_abstractions {
Abstraction::Abstraction(const shared_ptr<AbstractTask> &task, utils::LogProxy &log)
    : transition_system(utils::make_unique_ptr<TransitionSystem>(TaskProxy(*task).get_operators())),
      concrete_initial_state(TaskProxy(*task).get_initial_state()),
      goal_facts(task_properties::get_fact_pairs(TaskProxy(*task).get_goals())),
      refinement_hierarchy(utils::make_unique_ptr<RefinementHierarchy>(task)),
      log(log) {
    initialize_trivial_abstraction(get_domain_sizes(TaskProxy(*task)));
}

Abstraction::~Abstraction() {
}

const AbstractState &Abstraction::get_initial_state() const {
    return *states[init_id];
}

int Abstraction::get_num_states() const {
    return states.size();
}

const Goals &Abstraction::get_goals() const {
    return goals;
}

const AbstractState &Abstraction::get_state(int state_id) const {
    return *states[state_id];
}

const TransitionSystem &Abstraction::get_transition_system() const {
    return *transition_system;
}

unique_ptr<RefinementHierarchy> Abstraction::extract_refinement_hierarchy() {
    assert(refinement_hierarchy);
    return move(refinement_hierarchy);
}

void Abstraction::mark_all_states_as_goals() {
    goals.clear();
    for (auto &state : states) {
        goals.insert(state->get_id());
    }
}

void Abstraction::initialize_trivial_abstraction(const vector<int> &domain_sizes) {
    unique_ptr<AbstractState> init_state =
        AbstractState::get_trivial_abstract_state(domain_sizes);
    init_id = init_state->get_id();
    goals.insert(init_state->get_id());
    states.push_back(move(init_state));
}

pair<int, int> Abstraction::refine(
    const AbstractState &state, int var, const vector<int> &wanted) {
    if (log.is_at_least_debug())
        log << "Refine " << state << " for " << var << "=" << wanted << endl;

    int v_id = state.get_id();
    // Reuse state ID from obsolete parent to obtain consecutive IDs.
    int v1_id = v_id;
    int v2_id = get_num_states();

    // Update refinement hierarchy.
    pair<NodeID, NodeID> node_ids = refinement_hierarchy->split(
        state.get_node_id(), var, wanted, v1_id, v2_id);

    pair<CartesianSet, CartesianSet> cartesian_sets =
        state.split_domain(var, wanted);

    unique_ptr<AbstractState> v1 = utils::make_unique_ptr<AbstractState>(
        v1_id, node_ids.first, move(cartesian_sets.first));
    unique_ptr<AbstractState> v2 = utils::make_unique_ptr<AbstractState>(
        v2_id, node_ids.second, move(cartesian_sets.second));
    assert(state.includes(*v1));
    assert(state.includes(*v2));

    /*
      Due to the way we split the state into v1 and v2, v2 is never the new
      initial state and v1 is never a goal state.
    */
    if (state.get_id() == init_id) {
        if (v1->includes(concrete_initial_state)) {
            assert(!v2->includes(concrete_initial_state));
            init_id = v1_id;
        } else {
            assert(v2->includes(concrete_initial_state));
            init_id = v2_id;
        }
        if (log.is_at_least_debug()) {
            log << "New init state #" << init_id << ": " << get_state(init_id)
                << endl;
        }
    }
    if (goals.count(v_id)) {
        goals.erase(v_id);
        if (v1->includes(goal_facts)) {
            goals.insert(v1_id);
        }
        if (v2->includes(goal_facts)) {
            goals.insert(v2_id);
        }
        if (log.is_at_least_debug()) {
            log << "Goal states: " << goals.size() << endl;
        }
    }

    transition_system->rewire(states, v_id, *v1, *v2, var);

    states[v1_id] = move(v1);
    assert(static_cast<int>(states.size()) == v2_id);
    states.push_back(move(v2));

    return {v1_id, v2_id};
}

void Abstraction::print_statistics() const {
    if (log.is_at_least_normal()) {
        log << "States: " << get_num_states() << endl;
        log << "Goal states: " << goals.size() << endl;
        transition_system->print_statistics(log);
    }
}
}

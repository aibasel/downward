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

namespace cegar {
Abstraction::Abstraction(const shared_ptr<AbstractTask> &task, bool debug)
    : transition_system(utils::make_unique_ptr<TransitionSystem>(TaskProxy(*task).get_operators())),
      concrete_initial_state(TaskProxy(*task).get_initial_state()),
      goal_facts(task_properties::get_fact_pairs(TaskProxy(*task).get_goals())),
      init(nullptr),
      refinement_hierarchy(utils::make_unique_ptr<RefinementHierarchy>(task)),
      debug(debug) {
    initialize_trivial_abstraction(get_domain_sizes(TaskProxy(*task)));
}

Abstraction::~Abstraction() {
    for (AbstractState *state : states)
        delete state;
}

const AbstractStates &Abstraction::get_states() const {
    return states;
}

AbstractState *Abstraction::get_initial_state() const {
    return init;
}

int Abstraction::get_num_states() const {
    return states.size();
}

const Goals &Abstraction::get_goals() const {
    return goals;
}

AbstractState *Abstraction::get_state(int state_id) const {
    assert(utils::in_bounds(state_id, states));
    return states[state_id];
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
    for (const AbstractState *state : states) {
        goals.insert(state->get_id());
    }
}

void Abstraction::initialize_trivial_abstraction(const vector<int> &domain_sizes) {
    init = AbstractState::get_trivial_abstract_state(
        domain_sizes, refinement_hierarchy->get_root());
    goals.insert(init->get_id());
    states.push_back(init);
}

pair<int, int> Abstraction::refine(
    AbstractState *state, int var, const vector<int> &wanted) {
    if (debug)
        cout << "Refine " << *state << " for " << var << "=" << wanted << endl;

    int v_id = state->get_id();
    // Reuse state ID from obsolete parent to obtain consecutive IDs.
    int v1_id = v_id;
    int v2_id = get_num_states();
    pair<AbstractState *, AbstractState *> new_states = state->split(
        var, wanted, v1_id, v2_id);
    AbstractState *v1 = new_states.first;
    AbstractState *v2 = new_states.second;
    delete state;
    states[v1_id] = v1;
    assert(static_cast<int>(states.size()) == v2_id);
    states.push_back(v2);

    /*
      Due to the way we split the state into v1 and v2, v2 is never the new
      initial state and v1 is never a goal state.
    */
    if (state == init) {
        if (v1->includes(concrete_initial_state)) {
            assert(!v2->includes(concrete_initial_state));
            init = v1;
        } else {
            assert(v2->includes(concrete_initial_state));
            init = v2;
        }
        if (debug) {
            cout << "New init state #" << init->get_id() << ": " << *init << endl;
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
        if (debug) {
            cout << "Goal states: " << goals.size() << endl;
        }
    }

    transition_system->rewire(states, v_id, v1, v2, var);
    return {
               v1_id, v2_id
    };
}

void Abstraction::print_statistics() const {
    cout << "States: " << get_num_states() << endl;
    cout << "Goal states: " << goals.size() << endl;
    transition_system->print_statistics();
}
}

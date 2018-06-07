#include "cegar.h"

#include "abstraction.h"
#include "abstract_state.h"
#include "transition_system.h"
#include "utils.h"

#include "../globals.h"

#include "../task_utils/task_properties.h"
#include "../utils/language.h"
#include "../utils/logging.h"
#include "../utils/math.h"
#include "../utils/memory.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <unordered_map>

using namespace std;

namespace cegar {
struct Flaw {
    // Last concrete and abstract state reached while tracing solution.
    const State concrete_state;
    // TODO: After conversion to smart pointers, store as unique_ptr?
    AbstractState *current_abstract_state;
    // Hypothetical Cartesian set we would have liked to reach.
    const AbstractState desired_abstract_state;

    Flaw(
        State &&concrete_state,
        AbstractState *current_abstract_state,
        AbstractState &&desired_abstract_state)
        : concrete_state(move(concrete_state)),
          current_abstract_state(current_abstract_state),
          desired_abstract_state(move(desired_abstract_state)) {
        assert(this->current_abstract_state->includes(this->concrete_state));
    }

    vector<Split> get_possible_splits() const {
        vector<Split> splits;
        /*
          For each fact in the concrete state that is not contained in the
          desired abstract state, loop over all values in the domain of the
          corresponding variable. The values that are in both the current and
          the desired abstract state are the "wanted" ones, i.e., the ones that
          we want to split off.
        */
        for (FactProxy wanted_fact_proxy : concrete_state) {
            FactPair fact = wanted_fact_proxy.get_pair();
            if (!desired_abstract_state.contains(fact.var, fact.value)) {
                VariableProxy var = wanted_fact_proxy.get_variable();
                int var_id = var.get_id();
                vector<int> wanted;
                for (int value = 0; value < var.get_domain_size(); ++value) {
                    if (current_abstract_state->contains(var_id, value) &&
                        desired_abstract_state.contains(var_id, value)) {
                        wanted.push_back(value);
                    }
                }
                assert(!wanted.empty());
                splits.emplace_back(var_id, move(wanted));
            }
        }
        assert(!splits.empty());
        return splits;
    }
};

CEGAR::CEGAR(
    const shared_ptr<AbstractTask> &task,
    int max_states,
    int max_non_looping_transitions,
    double max_time,
    PickSplit pick,
    utils::RandomNumberGenerator &rng,
    bool debug)
    : task_proxy(*task),
      domain_sizes(get_domain_sizes(task_proxy)),
      max_states(max_states),
      max_non_looping_transitions(max_non_looping_transitions),
      split_selector(task, pick),
      abstraction(utils::make_unique_ptr<Abstraction>(task, debug)),
      abstract_search(task_properties::get_operator_costs(task_proxy)),
      timer(max_time),
      debug(debug) {
    assert(max_states >= 1);
    g_log << "Start building abstraction." << endl;
    cout << "Maximum number of states: " << max_states << endl;
    cout << "Maximum number of transitions: "
         << max_non_looping_transitions << endl;
    refinement_loop(rng);
    g_log << "Done building abstraction." << endl;
    cout << "Time for building abstraction: " << timer.get_elapsed_time() << endl;

    print_statistics();
}

CEGAR::~CEGAR() {
}

unique_ptr<Abstraction> CEGAR::extract_abstraction() {
    assert(abstraction);
    return move(abstraction);
}

void CEGAR::separate_facts_unreachable_before_goal() {
    assert(abstraction->get_goals().size() == 1);
    assert(abstraction->get_states().size() == 1);
    assert(task_proxy.get_goals().size() == 1);
    FactProxy goal = task_proxy.get_goals()[0];
    unordered_set<FactProxy> reachable_facts = get_relaxed_possible_before(
        task_proxy, goal);
    for (VariableProxy var : task_proxy.get_variables()) {
        if (!may_keep_refining())
            break;
        int var_id = var.get_id();
        vector<int> unreachable_values;
        for (int value = 0; value < var.get_domain_size(); ++value) {
            FactProxy fact = var.get_fact(value);
            if (reachable_facts.count(fact) == 0)
                unreachable_values.push_back(value);
        }
        if (!unreachable_values.empty())
            abstraction->refine(abstraction->get_initial_state(), var_id, unreachable_values);
    }
    abstraction->mark_all_states_as_goals();
}

bool CEGAR::may_keep_refining() const {
    return abstraction->get_num_states() < max_states &&
           abstraction->get_transition_system().get_num_non_loops() < max_non_looping_transitions &&
           !timer.is_expired() &&
           utils::extra_memory_padding_is_reserved();
}

void CEGAR::update_goal_distances(const Solution &solution) {
    int goal_distance = 0;
    for (auto it = solution.rbegin(); it != solution.rend(); ++it) {
        const Transition &transition = *it;
        AbstractState *current = abstraction->get_state(transition.target_id);
        current->set_goal_distance_estimate(goal_distance);
        int cost = task_proxy.get_operators()[transition.op_id].get_cost();
        goal_distance += cost;
    }
    abstraction->get_initial_state()->set_goal_distance_estimate(goal_distance);
}

void CEGAR::refinement_loop(utils::RandomNumberGenerator &rng) {
    /*
      For landmark tasks we have to map all states in which the
      landmark might have been achieved to arbitrary abstract goal
      states. For the other types of subtasks our method won't find
      unreachable facts, but calling it unconditionally for subtasks
      with one goal doesn't hurt and simplifies the implementation.
    */
    if (task_proxy.get_goals().size() == 1) {
        separate_facts_unreachable_before_goal();
    }
    bool found_concrete_solution = false;
    while (may_keep_refining()) {
        unique_ptr<Solution> solution = abstract_search.find_solution(
            abstraction->get_transition_system().get_outgoing_transitions(),
            abstraction->get_states(),
            abstraction->get_initial_state()->get_id(),
            abstraction->get_goals());
        if (!solution) {
            cout << "Abstract problem is unsolvable!" << endl;
            break;
        }
        update_goal_distances(*solution);
        unique_ptr<Flaw> flaw = find_flaw(*solution);
        if (!flaw) {
            found_concrete_solution = true;
            break;
        }
        AbstractState *abstract_state = flaw->current_abstract_state;
        vector<Split> splits = flaw->get_possible_splits();
        const Split &split = split_selector.pick_split(*abstract_state, splits, rng);
        abstraction->refine(abstract_state, split.var_id, split.values);
        if (abstraction->get_num_states() % 1000 == 0) {
            g_log << abstraction->get_num_states() << "/" << max_states << " states, "
                  << abstraction->get_transition_system().get_num_non_loops() << "/"
                  << max_non_looping_transitions << " transitions" << endl;
        }
    }
    cout << "Concrete solution found: " << found_concrete_solution << endl;
}

unique_ptr<Flaw> CEGAR::find_flaw(const Solution &solution) {
    if (debug)
        cout << "Check solution:" << endl;

    AbstractState *abstract_state = abstraction->get_initial_state();
    State concrete_state = task_proxy.get_initial_state();
    assert(abstract_state->includes(concrete_state));

    if (debug)
        cout << "  Initial abstract state: " << *abstract_state << endl;

    for (const Transition &step : solution) {
        if (!utils::extra_memory_padding_is_reserved())
            break;
        OperatorProxy op = task_proxy.get_operators()[step.op_id];
        AbstractState *next_abstract_state = abstraction->get_state(step.target_id);
        if (task_properties::is_applicable(op, concrete_state)) {
            if (debug)
                cout << "  Move to " << *next_abstract_state << " with "
                     << op.get_name() << endl;
            State next_concrete_state = concrete_state.get_successor(op);
            if (!next_abstract_state->includes(next_concrete_state)) {
                if (debug)
                    cout << "  Paths deviate." << endl;
                return utils::make_unique_ptr<Flaw>(
                    move(concrete_state),
                    abstract_state,
                    next_abstract_state->regress(op));
            }
            abstract_state = next_abstract_state;
            concrete_state = move(next_concrete_state);
        } else {
            if (debug)
                cout << "  Operator not applicable: " << op.get_name() << endl;
            return utils::make_unique_ptr<Flaw>(
                move(concrete_state),
                abstract_state,
                AbstractState::get_cartesian_set(
                    domain_sizes, op.get_preconditions()));
        }
    }
    assert(abstraction->get_goals().count(abstract_state->get_id()));
    if (task_properties::is_goal_state(task_proxy, concrete_state)) {
        // We found a concrete solution.
        return nullptr;
    } else {
        if (debug)
            cout << "  Goal test failed." << endl;
        return utils::make_unique_ptr<Flaw>(
            move(concrete_state),
            abstract_state,
            AbstractState::get_cartesian_set(
                domain_sizes, task_proxy.get_goals()));
    }
}

void CEGAR::print_statistics() {
    abstraction->print_statistics();
    cout << endl;
}
}

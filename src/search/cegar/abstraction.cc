#include "abstraction.h"

#include "abstract_state.h"
#include "utils.h"

#include "../option_parser.h"
#include "../task_tools.h"
#include "../timer.h"
#include "../utilities.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <unordered_map>

using namespace std;

namespace cegar {
struct Flaw {
    const State conc_state;
    AbstractState *abs_state;
    const AbstractState desired_abs_state;

    Flaw(State && conc_state, AbstractState *abs_state, AbstractState && desired_abs_state)
        : conc_state(conc_state),
          abs_state(abs_state),
          desired_abs_state(std::move(desired_abs_state)) {
    }
    vector<Split> get_possible_splits() const {
        vector<Split> splits;
        for (FactProxy wanted_fact : conc_state) {
            if (!abs_state->contains(wanted_fact) ||
                !desired_abs_state.contains(wanted_fact)) {
                VariableProxy var = wanted_fact.get_variable();
                vector<int> wanted;
                for (int value = 0; value < var.get_domain_size(); ++value) {
                    FactProxy fact = var.get_fact(value);
                    if (abs_state->contains(fact) && desired_abs_state.contains(fact)) {
                        wanted.push_back(value);
                    }
                }
                splits.emplace_back(var.get_id(), move(wanted));
            }
        }
        assert(!splits.empty());
        return splits;
    }
};

Abstraction::Abstraction(const Options &opts)
    : task_proxy(*opts.get<shared_ptr<AbstractTask> >("transform")),
      do_separate_unreachable_facts(opts.get<bool>("separate_unreachable_facts")),
      abstract_search(opts),
      split_selector(opts.get<shared_ptr<AbstractTask> >("transform"),
                     PickSplit(opts.get<int>("pick"))),
      max_states(opts.get<int>("max_states")),
      timer(opts.get<double>("max_time")),
      concrete_initial_state(task_proxy.get_initial_state()),
      init(nullptr),
      deviations(0),
      unmet_preconditions(0),
      unmet_goals(0) {
    Log() << "Start building abstraction.";
    build();
    Log() << "Done building abstraction.";

    // Even if we found a concrete solution, we might have refined in the
    // last iteration, so we must update the h values.
    update_h_values();

    print_statistics();
}

Abstraction::~Abstraction() {
    for (AbstractState *state : states)
        delete state;
}

bool Abstraction::is_goal(AbstractState *state) const {
    return goals.count(state) == 1;
}

void Abstraction::separate_unreachable_facts() {
    assert(states.size() == 1);
    FactProxy landmark = task_proxy.get_goals()[0];
    unordered_set<FactProxy> reachable_facts = get_relaxed_reachable_facts(
        task_proxy, landmark);
    for (VariableProxy var : task_proxy.get_variables()) {
        int var_id = var.get_id();
        vector<int> unreachable_values;
        for (int value = 0; value < var.get_domain_size(); ++value) {
            FactProxy fact = var.get_fact(value);
            if (reachable_facts.count(fact) == 0)
                unreachable_values.push_back(value);
        }
        if (!unreachable_values.empty())
            refine(init, var_id, unreachable_values);
    }
    goals.insert(states.begin(), states.end());
}

void Abstraction::create_trivial_abstraction() {
    init = AbstractState::get_trivial_abstract_state(task_proxy, split_tree.get_root());
    goals.insert(init);
    for (OperatorProxy op : task_proxy.get_operators()) {
        init->add_loop(op);
    }
    states.insert(init);
    if (do_separate_unreachable_facts)
        separate_unreachable_facts();
}

bool Abstraction::may_keep_refining() const {
    return memory_padding_is_reserved() &&
           get_num_states() < max_states &&
           !timer.is_expired();
}

void Abstraction::build() {
    create_trivial_abstraction();
    bool found_conc_solution = false;
    while (may_keep_refining()) {
        bool found_abs_solution = abstract_search.find_solution(init, goals);
        if (!found_abs_solution) {
            cout << "Abstract problem is unsolvable!" << endl;
            break;
        }
        const shared_ptr<Flaw> flaw = find_flaw(abstract_search.get_solution());
        if (!flaw) {
            found_conc_solution = true;
            break;
        }
        AbstractState *abs_state = flaw->abs_state;
        vector<Split> splits = flaw->get_possible_splits();
        const Split &split = split_selector.pick_split(*abs_state, splits);
        refine(abs_state, split.var_id, split.values);
    }
    cout << "Concrete solution found: " << found_conc_solution << endl;
}

void Abstraction::refine(AbstractState *state, int var, const vector<int> &wanted) {
    if (DEBUG)
        cout << "Refine " << *state << " for " << var << "=" << wanted << endl;
    pair<AbstractState *, AbstractState *> new_states = state->split(var, wanted);
    AbstractState *v1 = new_states.first;
    AbstractState *v2 = new_states.second;

    states.erase(state);
    states.insert(v1);
    states.insert(v2);

    // Since the search is always started from the abstract
    // initial state, v2 is never "init" and v1 is never "goal".
    if (state == init) {
        assert(v1->is_abstraction_of(concrete_initial_state));
        assert(!v2->is_abstraction_of(concrete_initial_state));
        init = v1;
        if (DEBUG)
            cout << "Using new init state: " << *init << endl;
    }
    if (is_goal(state)) {
        goals.erase(state);
        goals.insert(v2);
        if (DEBUG)
            cout << "Using new/additional goal state: " << *v2 << endl;
    }

    int num_states = get_num_states();
    if (num_states % STATES_LOG_STEP == 0)
        Log() << "Abstract states: " << num_states << "/" << max_states;

    delete state;
}

shared_ptr<Flaw> Abstraction::find_flaw(const Solution &solution) {
    if (DEBUG)
        cout << "Check solution:" << endl;

    AbstractState *abs_state = init;
    AbstractState *prev_abs_state = nullptr;
    State conc_state = concrete_initial_state;
    State prev_conc_state = concrete_initial_state;  // Arbitrary state.

    if (DEBUG)
        cout << "  Initial abstract state: " << *abs_state << endl;

    for (size_t step = 0; step < solution.size(); ++step) {
        if (!memory_padding_is_reserved())
            break;
        const OperatorProxy next_op = solution[step].first;
        AbstractState *next_abs_state = solution[step].second;
        if (!abs_state->is_abstraction_of(conc_state)) {
            assert(step >= 1);
            assert(prev_abs_state);
            if (DEBUG)
                cout << "  Paths deviate." << endl;
            ++deviations;
            OperatorProxy prev_op = solution[step - 1].first;
            return make_shared<Flaw>(move(prev_conc_state),
                                     prev_abs_state,
                                     abs_state->regress(prev_op));
        } else if (!is_applicable(next_op, conc_state)) {
            if (DEBUG)
                cout << "  Operator not applicable: " << next_op.get_name() << endl;
            ++unmet_preconditions;
            return make_shared<Flaw>(
                       move(conc_state),
                       abs_state,
                       AbstractState::get_abstract_state(task_proxy, next_op.get_preconditions()));
        } else {
            if (DEBUG)
                cout << "  Move to " << *next_abs_state << " with "
                     << next_op.get_name() << endl;
            prev_abs_state = abs_state;
            abs_state = next_abs_state;
            State next_conc_state = move(conc_state.apply(next_op));
            prev_conc_state = move(conc_state);
            conc_state = move(next_conc_state);
        }
    }
    assert(is_goal(abs_state));
    if (is_goal_state(task_proxy, conc_state)) {
        // We found a concrete solution.
        return nullptr;
    } else {
        // Get unmet goals and refine the last state.
        if (DEBUG)
            cout << "  Goal test failed." << endl;
        ++unmet_goals;
        return make_shared<Flaw>(
                   move(conc_state),
                   abs_state,
                   AbstractState::get_abstract_state(task_proxy, task_proxy.get_goals()));
    }
}

void Abstraction::update_h_values() {
    abstract_search.backwards_dijkstra(goals);
    for (AbstractState *state : states) {
        state->set_h(abstract_search.get_g(state));
    }
}

int Abstraction::get_init_h() const {
    return init->get_h();
}

vector<int> Abstraction::get_needed_costs() {
    return abstract_search.get_needed_costs(init, task_proxy.get_operators().size());
}

void Abstraction::print_statistics() {
    int total_incoming_arcs = 0;
    int total_outgoing_arcs = 0;
    int total_loops = 0;
    int dead_ends = 0;
    int arc_size = 0;
    for (AbstractState *state : states) {
        if (state->get_h() == INF)
            ++dead_ends;
        const Arcs &incoming_arcs = state->get_incoming_arcs();
        const Arcs &outgoing_arcs = state->get_outgoing_arcs();
        const Loops &loops = state->get_loops();
        total_incoming_arcs += incoming_arcs.size();
        total_outgoing_arcs += outgoing_arcs.size();
        total_loops += loops.size();
        arc_size += sizeof(incoming_arcs) + sizeof(Arc) * incoming_arcs.capacity() +
                    sizeof(outgoing_arcs) + sizeof(Arc) * outgoing_arcs.capacity() +
                    sizeof(loops) + sizeof(OperatorProxy) * loops.capacity();
    }
    assert(total_outgoing_arcs == total_incoming_arcs);

    int total_cost = 0;
    for (OperatorProxy op : task_proxy.get_operators())
        total_cost += op.get_cost();

    cout << "Time: " << timer << endl;
    cout << "Total operator cost: " << total_cost << endl;
    cout << "States: " << get_num_states() << endl;
    cout << "Dead ends: " << dead_ends << endl;
    cout << "Init-h: " << init->get_h() << endl;

    cout << "Transitions: " << total_incoming_arcs << endl;
    cout << "Self-loops: " << total_loops << endl;
    cout << "Transitions and self-loops size: " << arc_size / 1024 << " KB" << endl;

    cout << "Deviations: " << deviations << endl;
    cout << "Unmet preconditions: " << unmet_preconditions << endl;
    cout << "Unmet goals: " << unmet_goals << endl;
}
}

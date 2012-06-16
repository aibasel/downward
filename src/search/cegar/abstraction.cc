#include "abstraction.h"

#include "../globals.h"
#include "../operator.h"
#include "../priority_queue.h"
#include "../state.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

using namespace std;

namespace cegar_heuristic {

Abstraction::Abstraction() {
    assert(!g_operators.empty());
    single = new AbstractState();
    for (int i = 0; i < g_operators.size(); ++i) {
        single->add_arc(&g_operators[i], single);
    }
    init = single;
}

void Abstraction::refine(AbstractState *state, int var, int value) {
    assert(!g_operators.empty()); // We need operators and the g_initial_state
    if (DEBUG)
        cout << "REFINE " << state->str() << " for " << var << "=" << value << " (" << g_variable_name[var] << ")" << endl;
    AbstractState *v1 = new AbstractState();
    AbstractState *v2 = new AbstractState();
    state->refine(var, value, v1, v2);
    if (state == init) {
        if (v1->is_abstraction_of(*g_initial_state)) {
            init = v1;
        } else {
            assert(v2->is_abstraction_of(*g_initial_state));
            init = v2;
        }
        if (DEBUG)
            cout << "Using new init state: " << init->str() << endl;
    }
}

bool Abstraction::dijkstra_search(HeapQueue<AbstractState *> &queue, bool forward) {
    while (!queue.empty()) {
        pair<int, AbstractState *> top_pair = queue.pop();
        int distance = top_pair.first;
        AbstractState *state = top_pair.second;

        int state_distance = state->get_distance();
        if (DEBUG)
            cout << "VISIT: " << state->str() << " " << state_distance << " "
                 << distance << endl;
        assert(state_distance <= distance);
        if (state_distance < distance) {
            continue;
        }
        if (forward) {
            if (state->goal_reached()) {
                if (DEBUG)
                    cout << "GOAL REACHED" << endl;
                extract_solution(*state);
                return true;
            }
        }
        vector<Arc> successors = (forward) ? state->get_next() : state->get_prev();
        for (int i = 0; i < successors.size(); i++) {
            const Arc arc = successors[i];
            Operator *op = arc.first;
            AbstractState *successor = arc.second;

            int cost = op->get_cost();
            // Prevent overflow.
            int successor_cost = (state_distance == INFINITY) ? INFINITY : state_distance + cost;
            if (DEBUG)
                cout << "NEXT: " << successor->str() << " " << cost << " "
                     << successor_cost << " " << successor->get_distance() << endl;
            if (successor->get_distance() > successor_cost) {
                successor->set_distance(successor_cost);
                // TODO: Delete arc later to prevent memory leak.
                Arc *origin = new Arc(op, state);
                successor->set_origin(origin);
                if (DEBUG)
                    cout << "ORIGIN(" << successor->str() << ") = "
                         << state->str() << " with " << op->get_name() << endl;
                queue.push(successor_cost, successor);
            }
        }
    }
    return false;
}

bool Abstraction::find_solution() {
    HeapQueue<AbstractState *> queue;
    collect_states();
    for (int i = 0; i < abs_states.size(); ++i) {
        abs_states[i]->set_distance(INFINITY);
    }
    init->set_distance(0);
    init->set_origin(0);
    queue.push(0, init);
    return dijkstra_search(queue, true);
}

void Abstraction::extract_solution(AbstractState &goal) {
    solution_states.clear();
    solution_ops.clear();

    AbstractState *current = &goal;
    solution_states.push_front(current);
    while (current->get_origin()) {
        Operator *op = current->get_origin()->first;
        AbstractState *prev = current->get_origin()->second;
        solution_states.push_front(prev);
        solution_ops.push_front(op);
        assert(prev != current);
        current = prev;
    }
}

string Abstraction::get_solution_string() const {
    assert(!solution_states.empty());
    assert(solution_states.size() == solution_ops.size() + 1);
    string sep = "";
    ostringstream oss;
    oss << "[";
    for (int i = 0; i < solution_ops.size(); ++i) {
        oss << sep << solution_states[i]->str() << ","
            << solution_ops[i]->get_name();
        sep = ",";
    }
    oss << sep << solution_states[solution_states.size() - 1]->str();
    oss << "]";
    return oss.str();
}

bool Abstraction::check_solution() {
    assert(!solution_states.empty());
    assert(solution_states.size() == solution_ops.size() + 1);
    State conc_state = *g_initial_state;
    for (int i = 0; i < solution_states.size(); ++i) {
        AbstractState *abs_state = solution_states[i];
        if (DEBUG)
            cout << "Checking state " << i << ": " << abs_state->str() << endl;
        // Set next_op to null if there is no next operator.
        Operator *next_op = (i < solution_ops.size()) ? solution_ops[i] : 0;
        vector<pair<int, int> > unmet_cond;
        int var, value;
        if (!abs_state->is_abstraction_of(conc_state)) {
            // Get unmet conditions in previous state and refine it.
            assert(i >= 1);
            AbstractState *prev_state = solution_states[i - 1];
            AbstractState desired_prev_state;
            abs_state->regress(*solution_ops[i - 1], &desired_prev_state);
            // TODO: desired_prev_state might be null.
            prev_state->get_unmet_conditions(desired_prev_state, &unmet_cond);
            pick_condition(unmet_cond, &var, &value);
            refine(prev_state, var, value);
            return false;
        } else if (next_op && !next_op->is_applicable(conc_state)) {
            // Get unmet preconditions and refine the current state.
            get_unmet_preconditions(*solution_ops[i], conc_state, &unmet_cond);
            pick_condition(unmet_cond, &var, &value);
            refine(abs_state, var, value);
            return false;
        } else if (next_op) {
            // Go to the next concrete state.
            conc_state = State(conc_state, *next_op);
        } else if (!test_goal(conc_state)) {
            // Get unmet goals and refine the last state.
            get_unmet_goal_conditions(conc_state, &unmet_cond);
            pick_condition(unmet_cond, &var, &value);
            refine(abs_state, var, value);
            return false;
        } else {
            // We have reached the goal.
            return true;
        }
    }
    assert(false);
    return true;
}

void Abstraction::pick_condition(vector<pair<int, int> > &conditions, int *var, int *value) const {
    assert(!conditions.empty());
    if (DEBUG) {
        cout << "Unmet conditions: ";
        for (int i = 0; i < conditions.size(); ++i) {
            cout << conditions[i].first << "=" << conditions[i].second << " ";
        }
    cout << endl;
    }
    *var = conditions[0].first;
    *value = conditions[0].second;
}

void Abstraction::calculate_costs() {
    HeapQueue<AbstractState *> queue;
    collect_states();
    for (int i = 0; i < abs_states.size(); ++i) {
        if (abs_states[i]->goal_reached()) {
            abs_states[i]->set_distance(0);
        } else {
            abs_states[i]->set_distance(INFINITY);
        }
        queue.push(abs_states[i]->get_distance(), abs_states[i]);
    }
    dijkstra_search(queue, false);
}

AbstractState *Abstraction::get_abstract_state(const State &state) const {
    // TODO: Make current a pointer?
    AbstractState *current = single;
    while (!current->valid()) {
        int value = state[current->get_var()];
        current = current->get_child(value);
    }
    assert(current->valid());
    return current;
}

void Abstraction::collect_states() {
    abs_states.clear();
    collect_child_states(single);
}

void Abstraction::collect_child_states(AbstractState *parent) {
    if (parent->valid()) {
        abs_states.push_back(parent);
    } else {
        collect_child_states(parent->get_left_child());
        collect_child_states(parent->get_right_child());
    }
}
}

#include "abstraction.h"

#include "../globals.h"
#include "../operator.h"
#include "../priority_queue.h"
#include "../rng.h"
#include "../state.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

using namespace std;

namespace cegar_heuristic {
Abstraction::Abstraction(PickStrategy strategy) :
    last_checked_conc_state(*g_initial_state) {
    assert(!g_operators.empty());
    pick_strategy = strategy;
    single = new AbstractState();
    for (int i = 0; i < g_operators.size(); ++i) {
        //assert(g_operators[i].get_cost() > 0);
        single->add_arc(&g_operators[i], single);
    }
    init = single;
    goal = single;
    num_states = 1;
    start_solution_check_ptr = 0;
    dijkstra_searches = 0;
}

void Abstraction::refine(AbstractState *state, int var, int value) {
    assert(!g_operators.empty()); // We need operators and the g_initial_state
    if (DEBUG)
        cout << "REFINE " << state->str() << " for " << var << "=" << value << " (" << g_variable_name[var] << ")" << endl;
    AbstractState *v1 = new AbstractState();
    AbstractState *v2 = new AbstractState();
    start_solution_check_ptr = state->refine(var, value, v1, v2);
    ++num_states;
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
    if (state == goal) {
        if (v1->is_abstraction_of_goal()) {
            goal = v1;
        } else {
            assert(v2->is_abstraction_of_goal());
            goal = v2;
        }
        if (DEBUG)
            cout << "Using new goal state: " << goal->str() << endl;
    }
}

bool Abstraction::dijkstra_search(HeapQueue<AbstractState *> &queue, bool forward) {
    bool debug = false;
    ++dijkstra_searches;
    while (!queue.empty()) {
        pair<int, AbstractState *> top_pair = queue.pop();
        int &distance = top_pair.first;
        AbstractState *state = top_pair.second;

        int state_distance = state->get_distance();
        if (debug)
            cout << "VISIT: " << state->str() << " " << state_distance << " "
                 << distance << endl;
        assert(state_distance <= distance);
        if (state_distance < distance) {
            continue;
        }
        if (forward && (state == goal)) {
            if (debug)
                cout << "GOAL REACHED" << endl;
            extract_solution(*state);
            return true;
        }
        vector<Arc> &successors = (forward) ? state->get_next() : state->get_prev();
        for (int i = 0; i < successors.size(); i++) {
            const Arc &arc = successors[i];
            Operator *op = arc.first;
            AbstractState *successor = arc.second;

            const int &cost = op->get_cost();
            // Prevent overflow.
            int successor_cost = (state_distance == INFINITY) ? INFINITY : state_distance + cost;
            if (debug)
                cout << "NEXT: " << successor->str() << " " << cost << " "
                     << successor_cost << " " << successor->get_distance() << endl;
            if (successor->get_distance() > successor_cost) {
                successor->set_distance(successor_cost);
                Arc *origin = new Arc(op, state);
                successor->set_origin(origin);
                if (debug)
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
    AbstractState *current = &goal;
    while (current->get_origin()) {
        Operator *op = current->get_origin()->first;
        AbstractState *prev = current->get_origin()->second;
        prev->set_next_arc(new Arc(op, current));
        assert(prev != current);
        current = prev;
    }
}

string Abstraction::get_solution_string() const {
    string sep = "";
    ostringstream oss;
    oss << "[";
    AbstractState *current = init;
    oss << init->str();
    while (true) {
        Arc *arc = current->get_next_arc();
        if (!arc)
            break;
        Operator *op = arc->first;
        AbstractState *next_state = arc->second;
        oss << "," << op->get_name() << "," << next_state->str();
        current = next_state;
    }
    oss << "]";
    return oss.str();
}

bool Abstraction::check_solution() {
    if (DEBUG)
        cout << "Check solution." << endl;
    State conc_state = *g_initial_state;
    AbstractState *abs_state = init;
    if (DEBUG) {
        cout << "Start: " << start_solution_check_ptr;
        if (start_solution_check_ptr)
            cout << " " << start_solution_check_ptr->str();
        cout << endl;
    }
    if (start_solution_check_ptr) {
        abs_state = start_solution_check_ptr;
        conc_state = last_checked_conc_state;
    }
    assert(abs_state->is_abstraction_of(conc_state));

    AbstractState *prev_state = 0;
    Operator *prev_op = 0;
    State prev_last_checked_conc_state = last_checked_conc_state;
    while (true) {
        Arc *next_arc = abs_state->get_next_arc();
        // Set next_op to 0 if there is no next operator.
        Operator *next_op = (next_arc) ? next_arc->first : 0;
        vector<pair<int, int> > unmet_cond;
        int var, value;
        if (!abs_state->is_abstraction_of(conc_state)) {
            // Get unmet conditions in previous state and refine it.
            if (DEBUG)
                cout << "Concrete path deviates from abstract one." << endl;
            assert(prev_state);
            assert(prev_op);
            AbstractState desired_prev_state;
            abs_state->regress(*prev_op, &desired_prev_state);
            prev_state->get_unmet_conditions(desired_prev_state, &unmet_cond);
            pick_condition(unmet_cond, &var, &value);
            refine(prev_state, var, value);
            // Make sure we only reuse the solution if we are far enough from
            // the initial state to have saved the correct last-checked concrete
            // state.
            //start_solution_check_ptr = 0;
            if (start_solution_check_ptr) {
                if (!(last_checked_conc_state == prev_last_checked_conc_state)) {
                    last_checked_conc_state = prev_last_checked_conc_state;
                } else {
                    start_solution_check_ptr = 0;
                }
            }
            return false;
        } else if (next_op && !next_op->is_applicable(conc_state)) {
            // Get unmet preconditions and refine the current state.
            if (DEBUG)
                cout << "Operator is not applicable." << endl;
            get_unmet_preconditions(*next_op, conc_state, &unmet_cond);
            pick_condition(unmet_cond, &var, &value);
            refine(abs_state, var, value);
            return false;
        } else if (next_op) {
            // Go to the next state.
            prev_state = abs_state;
            prev_last_checked_conc_state = last_checked_conc_state;
            last_checked_conc_state = State(conc_state);
            prev_op = next_op;
            conc_state = State(conc_state, *next_op);
            abs_state = next_arc->second;
            if (DEBUG)
                cout << "Move to state " << abs_state->str() << endl;
        } else if (!test_goal(conc_state)) {
            // Get unmet goals and refine the last state.
            if (DEBUG)
                cout << "Goal test failed." << endl;
            assert(!abs_state->get_next_arc());
            get_unmet_goal_conditions(conc_state, &unmet_cond);
            pick_condition(unmet_cond, &var, &value);
            refine(abs_state, var, value);
            return false;
        } else {
            // We have reached the goal.
            assert(!abs_state->get_next_arc());
            return true;
        }
    }
    // This only happens if the problem is unsolvable.
    assert(false);
    return false;
}

void Abstraction::pick_condition(const vector<pair<int, int> > &conditions,
                                 int *var, int *value) const {
    assert(!conditions.empty());
    if (DEBUG) {
        cout << "Unmet conditions: ";
        for (int i = 0; i < conditions.size(); ++i) {
            cout << conditions[i].first << "=" << conditions[i].second << " ";
        }
        cout << endl;
    }
    int cond = -1;
    switch (pick_strategy) {
    case FIRST:
        cond = 0;
        break;
    case RANDOM:
        cond = g_rng.next(conditions.size());
        break;
    case GOAL:
        for (int i = 0; i < conditions.size(); ++i) {
            if (goal_var(conditions[i].first))
                cond = i;
            break;
        }
        if (cond == -1)
            cond = g_rng.next(conditions.size());
        break;
    default:
        cout << "Invalid pick strategy: " << pick_strategy << endl;
        exit(2);
    }
    *var = conditions[cond].first;
    *value = conditions[cond].second;
    if (DEBUG)
        cout << "Picked: " << *var << "=" << *value << endl;
}

void Abstraction::calculate_costs() {
    HeapQueue<AbstractState *> queue;
    collect_states();
    int num_goals = 0;
    for (int i = 0; i < abs_states.size(); ++i) {
        if (abs_states[i] == goal) {
            abs_states[i]->set_distance(0);
            queue.push(0, abs_states[i]);
            ++num_goals;
        } else {
            abs_states[i]->set_distance(INFINITY);
        }
    }
    // There can only be a single goal state, because we only refine the goal
    // state for vars that appear in it.
    assert(num_goals == 1);
    dijkstra_search(queue, false);
}

AbstractState *Abstraction::get_abstract_state(const State &state) const {
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

void Abstraction::write_dot_file(int num) {
    collect_states();
    ostringstream oss;
    oss << "graph-" << setw(3) << setfill('0') << num << ".dot";
    string filename = oss.str();
    ofstream dotfile(filename.c_str());
    if (!dotfile.is_open()) {
        cout << "File " << filename << " could not be opened" << endl;
        exit(1);
    }
    dotfile << "digraph abstract {" << endl;
    for (int i = 0; i < abs_states.size(); ++i) {
        AbstractState *current_state = abs_states[i];
        for (int j = 0; j < current_state->get_next().size(); ++j) {
            Arc arc = current_state->get_next()[j];
            AbstractState *next_state = arc.second;
            Operator *op = arc.first;
            dotfile << current_state->str() << " -> " << next_state->str()
                    << " [label=\"" << op->get_name() << "\"];" << endl;
        }
        if (current_state == init) {
            dotfile << current_state->str() << " [color=green];" << endl;
        } else if (current_state == goal) {
            dotfile << current_state->str() << " [color=red];" << endl;
        }
    }
    dotfile << "}" << endl;
    dotfile.close();
}
}

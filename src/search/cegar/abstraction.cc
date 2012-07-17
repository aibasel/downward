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
Abstraction::Abstraction(PickStrategy pick_deviation,
                         PickStrategy pick_precondition,
                         PickStrategy pick_goal)
    : last_checked_conc_state(*g_initial_state) {
    assert(!g_operators.empty());

    this->pick_deviation = pick_deviation;
    this->pick_precondition = pick_precondition;
    this->pick_goal = pick_goal;

    single = new AbstractState();
    for (int i = 0; i < g_operators.size(); ++i) {
        //assert(g_operators[i].get_cost() > 0);
        single->add_arc(&g_operators[i], single);
    }
    init = single;
    goal = single;
    states.insert(init);
    start_solution_check_ptr = 0;
    expansions = 0;
    expansions_dijkstra = 0;
    deviations = 0;
    unmet_preconditions = 0;
    unmet_goals = 0;
    if (g_causal_graph)
        partial_ordering(*g_causal_graph, &cg_partial_ordering);
    if (DEBUG) {
        cout << "Partial CG ordering: ";
        for (int pos = 0; pos < cg_partial_ordering.size(); ++pos) {
            cout << cg_partial_ordering[pos] << " ";
        }
        cout << endl;
    }
}

void Abstraction::refine(AbstractState *state, int var, int value) {
    assert(!g_operators.empty()); // We need operators and the g_initial_state
    if (DEBUG)
        cout << "REFINE " << state->str() << " for " << var << "=" << value << " (" << g_variable_name[var] << ")" << endl;
    AbstractState *v1 = new AbstractState();
    AbstractState *v2 = new AbstractState();
    start_solution_check_ptr = state->refine(var, value, v1, v2);
    states.erase(state);
    states.insert(v1);
    states.insert(v2);
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
            assert(!v2->is_abstraction_of_goal());
            goal = v1;
        } else {
            assert(v2->is_abstraction_of_goal());
            goal = v2;
        }
        if (DEBUG)
            cout << "Using new goal state: " << goal->str() << endl;
    }
}

void Abstraction::refine(vector<pair<int, int> > &conditions, AbstractState *state) {
    assert(!g_operators.empty()); // We need operators and the g_initial_state
    for (int cond = 0; cond < conditions.size(); ++cond) {
        int &var = conditions[cond].first;
        int &value = conditions[cond].second;
        refine(state, var, value);
        state = state->get_child(value);
        assert(state);
    }
}

void Abstraction::reset_distances() const {
    set<AbstractState *>::iterator it;
    for (it = states.begin(); it != states.end(); ++it) {
        (*it)->set_distance(INFINITY);
    }
}

bool Abstraction::dijkstra_search(HeapQueue<AbstractState *> &queue, bool forward) const {
    bool debug = DEBUG && false;
    while (!queue.empty()) {
        pair<int, AbstractState *> top_pair = queue.pop();
        int &distance = top_pair.first;
        AbstractState *state = top_pair.second;
        if (forward)
            ++expansions_dijkstra;

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
                Arc *prev_arc = new Arc(op, state);
                successor->set_prev_arc(prev_arc);
                if (debug)
                    cout << "ORIGIN(" << successor->str() << ") = "
                         << state->str() << " with " << op->get_name() << endl;
                queue.push(successor_cost, successor);
            }
        }
    }
    return false;
}

bool Abstraction::astar_search(HeapQueue<AbstractState *> &queue, bool forward,
                               bool use_h) const {
    bool debug = DEBUG && false;
    while (!queue.empty()) {
        pair<int, AbstractState *> top_pair = queue.pop();
        int &old_f = top_pair.first;
        AbstractState *state = top_pair.second;
        ++expansions;

        const int g = state->get_distance();
        int new_f = g;
        if (use_h)
            new_f += state->get_h();
        if (debug)
            cout << "VISIT: " << state->str() << " new_f=" << g << "+"
                 << state->get_h() << "=" << new_f << " old_f:" << old_f << endl;
        assert(new_f <= old_f);
        if (new_f < old_f) {
            continue;
        }
        if (state == goal) {
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
            int succ_g = (g == INFINITY) ? INFINITY : g + cost;
            // TODO: Ignore states with h = infinity?
            // TODO: In case of equal f-values, prefer states with higher g?

            if (debug)
                cout << "NEXT: " << successor->str() << " " << cost << " "
                     << succ_g << " " << successor->get_distance() << endl;
            if (successor->get_distance() > succ_g) {
                successor->set_distance(succ_g);
                int f = succ_g;
                if (use_h)
                    f += successor->get_h();
                Arc *prev_arc = new Arc(op, state);
                successor->set_prev_arc(prev_arc);
                if (debug)
                    cout << "f: " << f << " origin(" << successor->str() << ") = "
                         << state->str() << " with " << op->get_name() << endl;
                queue.push(f, successor);
            }
        }
    }
    return false;
}

bool Abstraction::find_solution() {
    HeapQueue<AbstractState *> queue;

    // Dijkstra.
    bool dijkstra_success = false;
    int dijkstra_cost = -1;
    if (TEST_WITH_DIJKSTRA) {
        queue.clear();
        reset_distances();
        init->set_distance(0);
        init->set_prev_arc(0);
        queue.push(0, init);
        dijkstra_success = dijkstra_search(queue, true);
        dijkstra_cost = init->get_h();
    }
    // A*.
    queue.clear();
    reset_distances();
    init->set_distance(0);
    init->set_prev_arc(0);
    queue.push(init->get_h(), init);
    bool astar_success = astar_search(queue, true, true);
    int astar_cost = init->get_h();

    if (TEST_WITH_DIJKSTRA) {
        assert(astar_success == dijkstra_success);
        assert(astar_cost == dijkstra_cost);
    }
    return astar_success;
}

void Abstraction::extract_solution(AbstractState &goal) const {
    int cost_to_goal = 0;
    AbstractState *current = &goal;
    current->set_h(cost_to_goal);
    while (current->get_prev_arc()) {
        Operator *op = current->get_prev_arc()->first;
        AbstractState *prev = current->get_prev_arc()->second;
        prev->set_next_arc(new Arc(op, current));
        cost_to_goal += op->get_cost();
        prev->set_h(cost_to_goal);
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
            ++deviations;
            assert(prev_state);
            assert(prev_op);
            AbstractState desired_prev_state;
            abs_state->regress(*prev_op, &desired_prev_state);
            prev_state->get_unmet_conditions(desired_prev_state, &unmet_cond);
            if (pick_deviation == ALL) {
                pick_condition_for_each_var(&unmet_cond);
                refine(unmet_cond, prev_state);
            } else {
                pick_condition(*prev_state, unmet_cond, pick_deviation, &var, &value);
                refine(prev_state, var, value);
            }
            // Make sure we only reuse the solution if we are far enough from
            // the initial state to have saved the correct last-checked concrete
            // state.
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
            ++unmet_preconditions;
            get_unmet_preconditions(*next_op, conc_state, &unmet_cond);
            if (pick_precondition == ALL) {
                refine(unmet_cond, abs_state);
            } else {
                pick_condition(*abs_state, unmet_cond, pick_precondition, &var, &value);
                refine(abs_state, var, value);
            }
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
            unmet_goals++;
            assert(!abs_state->get_next_arc());
            get_unmet_goal_conditions(conc_state, &unmet_cond);
            if (pick_goal == ALL) {
                refine(unmet_cond, abs_state);
            } else {
                pick_condition(*abs_state, unmet_cond, pick_goal, &var, &value);
                refine(abs_state, var, value);
            }
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

void Abstraction::pick_condition(AbstractState &state, const vector<pair<int, int> > &conditions,
                                 const PickStrategy &pick, int *var, int *value) const {
    assert(!conditions.empty());
    if (DEBUG) {
        cout << "Unmet conditions: ";
        for (int i = 0; i < conditions.size(); ++i) {
            cout << conditions[i].first << "=" << conditions[i].second << " ";
        }
        cout << endl;
    }
    // Shortcut for condition lists with only one element.
    if (conditions.size() == 1) {
        *var = conditions[0].first;
        *value = conditions[0].second;
        return;
    }
    int cond = -1;
    int random_cond = g_rng.next(conditions.size());
    if (pick == FIRST) {
        cond = 0;
    } else if (pick == RANDOM) {
        cond = random_cond;
    } else if (pick == GOAL or pick == NO_GOAL) {
        for (int i = 0; i < conditions.size(); ++i) {
            bool is_goal_var = goal_var(conditions[i].first);
            if ((pick == GOAL && is_goal_var) || (pick == NO_GOAL && !is_goal_var)) {
                cond = i;
                break;
            }
        }
        if (cond == -1)
            cond = random_cond;
    } else if (pick == MIN_CONSTRAINED || pick == MAX_CONSTRAINED) {
        int max_rest = -1;
        int min_rest = INFINITY;
        for (int i = 0; i < conditions.size(); ++i) {
            int rest = state.get_values(conditions[i].first).size();
            assert(rest >= 2);
            if (rest > max_rest && pick == MIN_CONSTRAINED) {
                cond = i;
                max_rest = rest;
            }
            if (rest < min_rest && pick == MAX_CONSTRAINED) {
                cond = i;
                min_rest = rest;
            }
        }
    } else if (pick == MIN_REFINED || pick == MAX_REFINED) {
        double min_refinement = 1.0;
        double max_refinement = -0.1;
        for (int i = 0; i < conditions.size(); ++i) {
            int all_values = g_variable_domain[conditions[i].first];
            int rest = state.get_values(conditions[i].first).size();
            assert(all_values >= 2);
            assert(rest >= 2);
            assert(rest <= all_values);
            // all=4, remaining=2 -> refinement=0.66
            // all=5, remaining=2 -> refinement=0.75
            double refinement = 1.0 - ((rest - 1.0) / (all_values - 1.0));
            assert(refinement < 1.0);
            assert(refinement >= 0.0);
            if (refinement < min_refinement && pick == MIN_REFINED) {
                cond = i;
                min_refinement = refinement;
            }
            if (refinement > max_refinement && pick == MAX_REFINED) {
                cond = i;
                max_refinement = refinement;
            }
        }
    } else if (pick == MIN_PREDECESSORS || pick == MAX_PREDECESSORS) {
        int min_pos = INFINITY;
        int max_pos = -1;
        for (int i = 0; i < conditions.size(); ++i) {
            int var = conditions[i].first;
            for (int pos = 0; pos < cg_partial_ordering.size(); ++pos) {
                if (var == cg_partial_ordering[pos]) {
                    if (pick == MIN_PREDECESSORS && pos < min_pos) {
                        cond = i;
                        min_pos = pos;
                    }
                    if (pick == MAX_PREDECESSORS && pos > max_pos) {
                        cond = i;
                        max_pos = pos;
                    }
                }
            }
        }
    } else if (pick == BREAK || pick == KEEP) {
        for (int i = 0; i < conditions.size(); ++i) {
            const int &var = conditions[i].first;
            const int &value = conditions[i].second;
            bool breaks = state.refinement_breaks_shortest_path(var, value);
            if ((pick == BREAK && breaks) || (pick == KEEP && !breaks))
                cond = i;
        }
        if (cond == -1)
            cond = random_cond;
    }else {
        cout << "Invalid pick strategy: " << pick << endl;
        exit(2);
    }
    assert(cond >= 0);
    *var = conditions[cond].first;
    *value = conditions[cond].second;
}

void Abstraction::calculate_costs() const {
    reset_distances();
    HeapQueue<AbstractState *> queue;
    queue.push(0, goal);
    goal->set_distance(0);
    dijkstra_search(queue, false);
}

void Abstraction::update_h_values() const {
    calculate_costs();
    set<AbstractState *>::iterator it;
    for (it = states.begin(); it != states.end(); ++it) {
        (*it)->set_h((*it)->get_distance());
    }
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

void Abstraction::write_dot_file(int num) {
    ostringstream oss;
    oss << "graph-" << setw(3) << setfill('0') << num << ".dot";
    string filename = oss.str();
    ofstream dotfile(filename.c_str());
    if (!dotfile.is_open()) {
        cout << "File " << filename << " could not be opened" << endl;
        exit(1);
    }
    dotfile << "digraph abstract {" << endl;
    set<AbstractState *>::iterator it;
    for (it = states.begin(); it != states.end(); ++it) {
        AbstractState *current_state = *it;
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

void Abstraction::release_memory() {
    vector<int>().swap(cg_partial_ordering);
    set<AbstractState *>::iterator it;
    for (it = states.begin(); it != states.end(); ++it) {
        AbstractState *state = *it;
        state->release_memory();
    }
}

void Abstraction::print_statistics() {
    set<AbstractState *>::iterator it;
    int nexts = 0, prevs = 0, loops = 0;
    for (it = states.begin(); it != states.end(); ++it) {
        AbstractState *state = *it;
        vector<Arc> &next = state->get_next();
        for (int i = 0; i < next.size(); ++i) {
            if (next[i].second == state)
                ++loops;
        }
        nexts += next.size();
        prevs += (&state->get_prev())->size();
    }
    assert(nexts == prevs);
    cout << "Arcs: " << nexts << endl;
    cout << "Self-loops: " << loops << endl;
    cout << "Deviations: " << deviations << endl;
    cout << "Unmet preconditions: " << unmet_preconditions << endl;
    cout << "Unmet goals: " << unmet_goals << endl;
}
}

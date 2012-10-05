#include "abstraction.h"

#include "abstract_state.h"
#include "../globals.h"
#include "../operator.h"
#include "../priority_queue.h"
#include "../rng.h"

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
Abstraction::Abstraction(PickStrategy deviation_strategy,
                         PickStrategy precondition_strategy,
                         PickStrategy goal_strategy)
    : pick_deviation(deviation_strategy),
      pick_precondition(precondition_strategy),
      pick_goal(goal_strategy),
      expansions(0),
      deviations(0),
      unmet_preconditions(0),
      unmet_goals(0),
      num_states_offline(-1),
      last_avg_h(0),
      last_init_h(0),
      use_astar(true),
      use_new_arc_check(true),
      log_h(false),
      memory_released(false) {
    assert(!g_operators.empty());

    single = new AbstractState();
    for (int i = 0; i < g_operators.size(); ++i) {
        single->add_loop(&g_operators[i]);
    }
    init = single;
    goal = single;
    states.insert(init);
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

void Abstraction::break_solution(AbstractState *state, int var, int value) {
    while (true) {
        refine(state, var, value);
        AbstractState *v1 = state->get_left_child();
        AbstractState *v2 = state->get_right_child();
        if (v1->get_state_in() && v2->get_state_in()) {
            // The arc on the solution u->v is now ambiguous (u->v1 and u->v2 exist).
            assert(v1->get_op_in() && v2->get_op_in());
            assert(v1->get_op_in() == v2->get_op_in());
            assert(v1->get_state_in() == v2->get_state_in());
            state = v1->get_state_in();
        } else {
            break;
        }
    }
    // We cannot calculate the avg_h value after each refinement, because the
    // solution pointers are reset in update_h_values() called by get_avg_h().
    // Instead we calculate it after a solution has been broken.
    log_h_values();
}

void Abstraction::refine(AbstractState *state, int var, int value) {
    assert(!g_operators.empty());  // We need operators and the g_initial_state.
    if (DEBUG)
        cout << "Refine " << state->str() << " for " << var << "=" << value
             << " (" << g_variable_name[var] << "=" << g_fact_names[var][value]
             << ")" << endl;
    AbstractState *v1 = new AbstractState();
    AbstractState *v2 = new AbstractState();
    state->refine(var, value, v1, v2, use_new_arc_check);
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

void Abstraction::improve_h(const State &state, AbstractState *abs_state) {
    int rounds = 0;
    const int old_h = abs_state->get_h();
    while (abs_state->get_h() == old_h && may_keep_refining_online()) {
        // Loop until the heuristic value increases.
        // TODO: Reuse last solution if still valid.
        bool solution_found = find_solution(abs_state);
        if (!solution_found) {
            cout << "No solution found" << endl;
            break;
        }
        bool solution_valid = check_solution(state, abs_state);
        if (solution_valid) {
            cout << "Concrete solution found" << endl;
            break;
        }
        // TODO: Use A* for finding the shortest path from abs_state to goal.
        update_h_values();
        abs_state = get_abstract_state(state);
        ++rounds;
    }
    assert(abs_state->get_h() >= old_h);
    if (DEBUG)
        cout << "Refinement rounds: " << rounds << endl;
}

void Abstraction::reset_distances() const {
    set<AbstractState *>::iterator it;
    for (it = states.begin(); it != states.end(); ++it) {
        AbstractState *state = (*it);
        state->set_distance(INFINITY);
    }
}

bool Abstraction::astar_search(bool forward, bool use_h) const {
    bool debug = DEBUG && false;
    while (!queue.empty()) {
        pair<int, AbstractState *> top_pair = queue.pop();
        int &old_f = top_pair.first;
        AbstractState *state = top_pair.second;
        ++expansions;

        if (!state->valid())
            continue;

        const int g = state->get_distance();
        assert(g < INFINITY);
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
        if (forward && state == goal) {
            if (debug)
                cout << "GOAL REACHED" << endl;
            extract_solution(goal);
            return true;
        }
        Arcs &successors = (forward) ? state->get_next() : state->get_prev();
        for (Arcs::iterator it = successors.begin(); it != successors.end(); ++it) {
            Operator *op = it->first;
            AbstractState *successor = it->second;

            const int succ_g = g + op->get_cost();
            if (successor->get_distance() > succ_g) {
                successor->set_distance(succ_g);
                int f = succ_g;
                int h = successor->get_h();
                if (use_h) {
                    // Ignore dead-end states.
                    if (h == INFINITY)
                        continue;
                    f += h;
                }
                successor->set_predecessor(op, state);
                assert(f >= 0);
                queue.push(f, successor);
            }
        }
    }
    return false;
}

bool Abstraction::find_solution(AbstractState *start) {
    if (!start)
        start = init;

    bool success = false;
    queue.clear();
    reset_distances();
    start->reset_neighbours();
    start->set_distance(0);

    if (use_astar) {
        // A*.
        queue.push(start->get_h(), start);
        success = astar_search(true, true);
    } else {
        // Dijkstra.
        queue.push(0, start);
        success = astar_search(true, false);
    }
    return success;
}

void Abstraction::extract_solution(AbstractState *goal) const {
    int cost_to_goal = 0;
    AbstractState *current = goal;
    current->set_h(cost_to_goal);
    while (current->get_state_in()) {
        Operator *op = current->get_op_in();
        assert(op);
        AbstractState *prev = current->get_state_in();
        prev->set_successor(op, current);
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
        AbstractState *next_state = current->get_state_out();
        if (!next_state)
            break;
        Operator *op = current->get_op_out();
        assert(op);
        oss << "," << op->get_name() << "," << next_state->str();
        current = next_state;
    }
    oss << "]";
    return oss.str();
}

bool Abstraction::check_solution(State conc_state, AbstractState *abs_state) {
    if (DEBUG)
        cout << "Check solution." << endl;
    if (!abs_state)
        abs_state = get_abstract_state(conc_state);
    assert(abs_state->is_abstraction_of(conc_state));

    AbstractState *prev_state = 0;
    Operator *prev_op = 0;

    // Initialize with arbitrary states.
    State prev_conc_state(*g_initial_state);

    while (true) {
        // next_op is 0 if there is no next operator.
        Operator *next_op = abs_state->get_op_out();
        AbstractState *next_state = abs_state->get_state_out();
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
            prev_state->get_unmet_conditions(desired_prev_state, prev_conc_state,
                                             &unmet_cond);
            pick_condition(*prev_state, unmet_cond, pick_deviation, &var, &value);
            break_solution(prev_state, var, value);
            return false;
        } else if (next_op && !next_op->is_applicable(conc_state)) {
            // Get unmet preconditions and refine the current state.
            if (DEBUG)
                cout << "Operator is not applicable: " << next_op->get_name() << endl;
            ++unmet_preconditions;
            get_unmet_preconditions(*next_op, conc_state, &unmet_cond);
            pick_condition(*abs_state, unmet_cond, pick_precondition, &var, &value);
            break_solution(abs_state, var, value);
            return false;
        } else if (next_op) {
            // Go to the next state.
            prev_state = abs_state;
            prev_conc_state = State(conc_state);
            prev_op = next_op;
            conc_state = State(conc_state, *next_op);
            assert(next_state);
            abs_state = next_state;
            if (DEBUG)
                cout << "Move to state " << abs_state->str() << " with "
                     << next_op->get_name() << endl;
        } else if (!test_goal(conc_state)) {
            // Get unmet goals and refine the last state.
            if (DEBUG)
                cout << "Goal test failed." << endl;
            unmet_goals++;
            assert(!next_state);
            get_unmet_goal_conditions(conc_state, &unmet_cond);
            pick_condition(*abs_state, unmet_cond, pick_goal, &var, &value);
            break_solution(abs_state, var, value);
            return false;
        } else {
            // We have reached the goal.
            assert(!next_state);
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
            int rest = state.count(conditions[i].first);
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
            int rest = state.count(conditions[i].first);
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
    } else {
        cout << "Invalid pick strategy: " << pick << endl;
        exit(2);
    }
    assert(cond >= 0);
    *var = conditions[cond].first;
    *value = conditions[cond].second;
}

void Abstraction::calculate_costs() const {
    reset_distances();
    queue.clear();
    queue.push(0, goal);
    goal->set_distance(0);
    astar_search(false, false);
}

void Abstraction::update_h_values() const {
    calculate_costs();
    set<AbstractState *>::iterator it;
    for (it = states.begin(); it != states.end(); ++it) {
        AbstractState *state = *it;
        state->set_h(state->get_distance());
    }
}

double Abstraction::get_avg_h() const {
    // 2 states with h=8, 3 states with h=6   -> avg_h = 28/5
    // 2 states with h=8, 3 states with h=inf -> avg_h = 28/5
    update_h_values();
    double avg_h = 0.;
    set<AbstractState *>::iterator it;
    for (it = states.begin(); it != states.end(); ++it) {
        AbstractState *state = *it;
        const int h = state->get_h();
        assert(h >= 0);
        if (h != INFINITY) {
            avg_h += h * state->get_rel_conc_states();
        }
    }
    assert(avg_h >= 0.);
    return avg_h;
}

void Abstraction::log_h_values() const {
    //double avg_h = get_avg_h();
    int init_h = log_h ? init->get_h() : 0;
    // We cannot assert(avg_h >= last_avg_h), because due to dead-end states,
    // which we don't count for the average h-value, the average h-value may
    // have decreased.
    if (//(avg_h > last_avg_h + PRECISION) ||
        (init_h > last_init_h)) {
        cout << "States: " << get_num_states() << ", avg-h: " << get_avg_h()
             << ", init-h: " << init_h << endl;
        //last_avg_h = avg_h;
        last_init_h = init_h;
    }
}

AbstractState *Abstraction::get_abstract_state(const State &state) const {
    AbstractState *current = single;
    while (!current->valid()) {
        int value = state[current->get_refined_var()];
        current = current->get_child(value);
    }
    assert(current->valid());
    // We cannot assert that current is an abstraction of state, because its
    // members have been cleared to save memory.
    return current;
}

void Abstraction::write_dot_file(int num) {
    bool draw_loops = false;
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
        Arcs &next = current_state->get_next();
        for (Arcs::iterator it = next.begin(); it != next.end(); ++it) {
            Operator *op = it->first;
            AbstractState *next_state = it->second;
            if (!next_state->valid())
                continue;
            dotfile << current_state->str() << " -> " << next_state->str()
                    << " [label=\"" << op->get_name() << "\"];" << endl;
        }
        if (draw_loops) {
            Loops &loops = current_state->get_loops();
            for (Loops::iterator it = loops.begin(); it != loops.end(); ++it) {
                Operator *op = *it;
                dotfile << current_state->str() << " -> " << current_state->str()
                        << " [label=\"" << op->get_name() << "\"];" << endl;
            }
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

int Abstraction::get_num_states_online() const {
    assert(num_states_offline >= 0);
    return states.size() - num_states_offline;
}

bool Abstraction::may_keep_refining_online() const {
    return get_num_states_online() < g_cegar_abstraction_max_states_online;
}

void Abstraction::release_memory() {
    assert(!memory_released);
    vector<int>().swap(cg_partial_ordering);
    set<AbstractState *>::iterator it;
    for (it = states.begin(); it != states.end(); ++it) {
        AbstractState *state = *it;
        state->release_memory();
    }
    memory_released = true;
}

void Abstraction::print_statistics() {
    int nexts = 0, prevs = 0, total_loops = 0;
    int unreachable_states = 0;
    int arc_size = 0;
    set<AbstractState *>::iterator it;
    for (it = states.begin(); it != states.end(); ++it) {
        AbstractState *state = *it;
        if (state->get_h() == INFINITY)
            ++unreachable_states;
        Arcs &next = state->get_next();
        Arcs &prev = state->get_prev();
        Loops &loops = state->get_loops();
        nexts += next.size();
        prevs += prev.size();
        total_loops += loops.size();
        arc_size += sizeof(next) + sizeof(Arc) * next.capacity() +
                    sizeof(prev) + sizeof(Arc) * prev.capacity() +
                    sizeof(loops) + sizeof(Operator*) * loops.capacity();
    }
    // Since we don't remove arcs to refined states anymore we cannot make the
    // following assumption: assert(nexts == prevs);

    int facts = 0;
    for (int var = 0; var < g_variable_domain.size(); ++var) {
        facts += g_variable_domain[var];
    }
    // Each bitset takes about 32 B.
    int bitset_bytes_single = get_num_states() * ((facts / 8) + 32);

    cout << "Arcs: " << nexts << endl;
    cout << "Self-loops: " << total_loops << endl;
    cout << "Deviations: " << deviations << endl;
    cout << "Unmet preconditions: " << unmet_preconditions << endl;
    cout << "Unmet goals: " << unmet_goals << endl;
    cout << "Unreachable states: " << unreachable_states << endl;
    cout << "Bitset size single: " << bitset_bytes_single / 1024 << " KB" << endl;
    cout << "Arc size: " << arc_size / 1024 << " KB" << endl;
    cout << "Arc size diff 2byte ops: " << (nexts + prevs + total_loops) * 2 / 1024 << " KB" << endl;
    cout << "Init h: " << init->get_h() << endl;
    cout << "Average h: " << get_avg_h() << endl;
}
}

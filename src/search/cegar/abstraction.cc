#include "abstraction.h"

#include "abstract_state.h"
#include "utils.h"
#include "../globals.h"
#include "../operator.h"
#include "../rng.h"
#include "../successor_generator.h"
#include "../timer.h"

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

Abstraction::Abstraction()
    : single(new AbstractState()),
      init(single),
      goal(single),
      queue(new AdaptiveQueue<AbstractState *>()),
      pick(RANDOM),
      rng(2012),
      deviations(0),
      unmet_preconditions(0),
      unmet_goals(0),
      num_states_offline(-1),
      last_avg_h(0),
      last_init_h(0),
      max_states_offline(1),
      max_states_online(0),
      max_time(INFINITY),
      use_astar(true),
      use_new_arc_check(true),
      log_h(false),
      memory_released(false) {

    assert(!g_memory_buffer);
    g_memory_buffer = new char [10 * 1024 * 1024];

    split_tree.set_root(single);
    for (int i = 0; i < g_operators.size(); ++i) {
        single->add_loop(&g_operators[i]);
    }
    states.insert(init);
    if (g_causal_graph)
        partial_ordering(*g_causal_graph, &cg_partial_ordering);
    if (DEBUG) {
        cout << "Causal graph ordering: ";
        for (int pos = 0; pos < cg_partial_ordering.size(); ++pos) {
            cout << cg_partial_ordering[pos] << " ";
        }
        cout << endl;
        cout << "Variable domain sizes: ";
        for (int var = 0; var < g_variable_domain.size(); ++var) {
            cout << var << ":" << g_variable_domain[var] << " ";
        }
        cout << endl;
    }
}

Abstraction::~Abstraction() {
    if (!memory_released)
        release_memory();
}

void Abstraction::build(int h_updates) {
    int updates = 0;
    int update_step = DEFAULT_H_UPDATE_STEP;
    if (h_updates >= 0) {
        update_step = (max_states_offline / (h_updates + 1)) + 1;
    } else if (max_states_offline >= 0) {
        h_updates = max_states_offline / update_step;
    }
    bool valid_complete_conc_solution = false;
    if (WRITE_DOT_FILES) {
        write_causal_graph(*g_causal_graph);
        assert(get_num_states() == 1);
        write_dot_file(get_num_states());
    }
    while (may_keep_refining()) {
        valid_complete_conc_solution = find_and_break_solution();
        if (valid_complete_conc_solution)
            break;
        // Update costs to goal evenly distributed over time.
        if (get_num_states() >= (updates + 1) * update_step) {
            update_h_values();
            ++updates;
        }
    }
    log_h_values();
    // Remember how many states where refined offline.
    num_states_offline = states.size();
    cout << "Done building abstraction [t=" << g_timer << "]" << endl;
    cout << "Peak memory after building abstraction: "
         << get_memory_in_kb("VmPeak") << " KB" << endl;
    cout << "Solution found while refining: " << valid_complete_conc_solution << endl;
    cout << "Abstract states offline: " << get_num_states() << endl;
    cout << "Cost updates: " << updates << "/" << h_updates << endl;
    update_h_values();
    print_statistics();
    if (max_states_online <= 0)
        release_memory();
}

void Abstraction::break_solution(AbstractState *state, const Splits &splits) {
    if (DEBUG) {
        cout << "Unmet conditions: ";
        for (int i = 0; i < splits.size(); ++i) {
            cout << splits[i].first << "=" << to_string(splits[i].second) << " ";
        }
        cout << endl;
    }
    int i = pick_split_index(*state, splits);
    assert(i >= 0 && i < splits.size());
    int var = splits[i].first;
    const vector<int> &wanted = splits[i].second;
    while (may_keep_refining()) {
        Node *node = state->get_node();
        refine(state, var, wanted);
        AbstractState *v1 = node->get_left_child_state();
        AbstractState *v2 = node->get_right_child_state();
        // If the solution is still valid after we substitute BOTH v1 and v2
        // for v, the same split can also be made in u.
        // TODO: How to generalize for wanted.size() > 1?
        if (wanted.size() == 1 && v1->get_state_in() && v2->get_state_in()) {
            // The arc on the solution u->v is now ambiguous (u->v1 and u->v2 exist).
            assert(v1->get_op_in() && v2->get_op_in());
            assert(v1->get_op_in() == v2->get_op_in());
            assert(v1->get_state_in() == v2->get_state_in());
            state = v1->get_state_in();
        } else {
            break;
        }
    }
    log_h_values();
}

void Abstraction::refine(AbstractState *state, int var, const vector<int> &wanted) {
    assert(may_keep_refining());
    if (DEBUG)
        cout << "Refine " << state->str() << " for "
             << var << "=" << to_string(wanted) << endl;
    AbstractState *v1 = new AbstractState();
    AbstractState *v2 = new AbstractState();
    Node *node = state->get_node();
    assert(node);
    state->split(var, wanted, v1, v2, use_new_arc_check);

    // Update split tree.
    node->split(var, wanted, v1, v2);
    assert(v1->get_node());
    assert(v2->get_node());

    int h = node->get_h();
    v1->set_h(h);
    v2->set_h(h);

    states.erase(state);
    states.insert(v1);
    states.insert(v2);

    // TODO: Remove some checks once we always start from the initial state again.
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

    delete state;
    state = 0;

    int num_states = get_num_states();
    if (num_states % STATES_LOG_STEP == 0)
        cout << "Abstract states: " << num_states << "/" << max_states_offline << endl;
    if (WRITE_DOT_FILES)
        write_dot_file(num_states);
}

AbstractState *Abstraction::improve_h(const State &state, AbstractState *abs_state) {
    int rounds = 0;
    const int old_h = abs_state->get_h();
    // Loop until the heuristic value increases.
    while (abs_state->get_h() == old_h && may_keep_refining()) {
        bool solution_found = find_solution(abs_state);
        if (!solution_found) {
            cout << "No abstract solution found" << endl;
            break;
        }
        bool solution_valid = check_and_break_solution(state, abs_state);
        if (solution_valid) {
            cout << "Concrete solution found" << endl;
            break;
        }
        abs_state = get_abstract_state(state);
        if (get_num_states() % DEFAULT_H_UPDATE_STEP == 0) {
            // Don't update_h_values in each round.
            update_h_values();
        } else {
            // Normally, we use A* for finding the shortest path from abs_state
            // to the goal since that's much faster than update_h_values().
            // If a solution is found, the h-values of all states on the path
            // are updated by the extract_solution function.
            solution_found = find_solution(abs_state);
            if (!solution_found)
                abs_state->set_h(INFINITY);
        }
        ++rounds;
    }
    assert(abs_state->get_h() >= old_h);
    if (DEBUG)
        cout << "Refinement rounds: " << rounds << endl;
    return abs_state;
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
    while (!queue->empty()) {
        pair<int, AbstractState *> top_pair = queue->pop();
        int &old_f = top_pair.first;
        AbstractState *state = top_pair.second;

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
                queue->push(f, successor);
            }
        }
    }
    return false;
}

bool Abstraction::find_solution(AbstractState *start) {
    if (!start)
        start = init;

    bool success = false;
    queue->clear();
    reset_distances();
    start->reset_neighbours();
    start->set_distance(0);

    if (use_astar) {
        // A*.
        queue->push(start->get_h(), start);
        success = astar_search(true, true);
    } else {
        // Dijkstra.
        queue->push(0, start);
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

bool Abstraction::find_and_break_solution() {
    // Start with initial state.
    bool solution_found = find_solution(init);
    // Suppress compiler warning about unused variable.
    if (!solution_found) {
        cout << "Abstract problem is unsolvable!" << endl;
        exit(0);
    }
    return check_and_break_solution(*g_initial_state, init);
}

bool Abstraction::check_and_break_solution(State conc_state, AbstractState *abs_state) {
    if (!abs_state)
        abs_state = get_abstract_state(conc_state);
    assert(abs_state->is_abstraction_of(conc_state));

    if (DEBUG)
        cout << "Check solution." << endl << "Start at      " << abs_state->str()
             << " (is init: " << (abs_state == init) << ")" << endl;

    AbstractState *prev_state = 0;
    Operator *prev_op = 0;
    Operator *next_op = abs_state->get_op_out();

    // Initialize with arbitrary states.
    State prev_conc_state(*g_initial_state);

    Splits splits;

    while (true) {
        if (!abs_state->is_abstraction_of(conc_state)) {
            // Get unmet conditions in previous state and refine it.
            if (DEBUG)
                cout << "Concrete path deviates from abstract one." << endl;
            ++deviations;
            assert(prev_state);
            assert(prev_op);
            AbstractState desired_prev_state;
            abs_state->regress(*prev_op, &desired_prev_state);
            prev_state->get_possible_splits(desired_prev_state, prev_conc_state,
                                            &splits);
            break_solution(prev_state, splits);
            return false;
        } else if (next_op && !next_op->is_applicable(conc_state)) {
            // Get unmet preconditions and refine the current state.
            if (DEBUG)
                cout << "Operator is not applicable: " << next_op->get_name() << endl;
            ++unmet_preconditions;
            get_unmet_preconditions(*next_op, conc_state, &splits);
            break_solution(abs_state, splits);
            return false;
        } else if (next_op) {
            // Go to the next state.
            assert(abs_state->get_state_out());
            if (DEBUG)
                cout << "Move to state " << abs_state->get_state_out()->str()
                     << " with " << next_op->get_name() << endl;
            prev_state = abs_state;
            prev_conc_state = State(conc_state);
            prev_op = next_op;
            conc_state = State(conc_state, *next_op);
            abs_state = abs_state->get_state_out();
            next_op = abs_state->get_op_out();
        } else {
            break;
        }
    }
    assert(abs_state == goal);
    assert(!abs_state->get_op_out());
    assert(!abs_state->get_state_out());

    if (test_goal(conc_state)) {
        // We have reached the goal.
        return true;
    }

    // Get unmet goals and refine the last state.
    if (DEBUG)
        cout << "Goal test failed." << endl;
    unmet_goals++;
    get_unmet_goal_conditions(conc_state, &splits);
    break_solution(abs_state, splits);
    return false;
}

int Abstraction::pick_split_index(AbstractState &state, const Splits &splits) const {
    assert(!splits.empty());
    // Shortcut for condition lists with only one element.
    if (splits.size() == 1) {
        return 0;
    }
    int cond = -1;
    int random_cond = rng.next(splits.size());
    if (pick == FIRST) {
        cond = 0;
    } else if (pick == RANDOM) {
        cond = random_cond;
    } else if (pick == GOAL or pick == NO_GOAL) {
        for (int i = 0; i < splits.size(); ++i) {
            bool is_goal_var = goal_var(splits[i].first);
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
        for (int i = 0; i < splits.size(); ++i) {
            int rest = state.count(splits[i].first);
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
        double min_refinement = 0.0;
        double max_refinement = -1.1;
        for (int i = 0; i < splits.size(); ++i) {
            int all_values = g_variable_domain[splits[i].first];
            int rest = state.count(splits[i].first);
            assert(all_values >= 2);
            assert(rest >= 2);
            assert(rest <= all_values);
            double refinement = - (rest / double(all_values));
            assert(refinement >= -1.0);
            assert(refinement < 0.0);
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
        for (int i = 0; i < splits.size(); ++i) {
            int var = splits[i].first;
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
    return cond;
}

void Abstraction::update_h_values() const {
    reset_distances();
    queue->clear();
    queue->push(0, goal);
    goal->set_distance(0);
    astar_search(false, false);
    set<AbstractState *>::iterator it;
    for (it = states.begin(); it != states.end(); ++it) {
        AbstractState *state = *it;
        state->set_h(state->get_distance());
    }
}

double Abstraction::get_avg_h() const {
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
    int init_h = log_h ? init->get_h() : 0;
    // We cannot assert(avg_h >= last_avg_h), because due to dead-end states,
    // which we don't count for the average h-value, the average h-value may
    // have decreased.
    if (init_h > last_init_h) {
        cout << "States: " << get_num_states() << ", avg-h: " << get_avg_h()
             << ", init-h: " << init_h << endl;
        last_init_h = init_h;
    }
}

AbstractState *Abstraction::get_abstract_state(const State &state) const {
    return split_tree.get_node(state)->get_abs_state();
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

bool Abstraction::may_keep_refining() const {
    return (g_memory_buffer &&
            (is_online() || get_num_states() < max_states_offline) &&
            (!is_online() || get_num_states_online() < max_states_online) &&
            (max_time == INFINITY || is_online() || timer() < max_time));
}

void Abstraction::release_memory() {
    cout << "Release memory" << endl;
    assert(!memory_released);
    vector<int>().swap(cg_partial_ordering);
    delete queue;
    queue = 0;
    if (g_memory_buffer) {
        delete[] g_memory_buffer;
        g_memory_buffer = 0;
    }
    set<AbstractState *>::iterator it;
    for (it = states.begin(); it != states.end(); ++it) {
        AbstractState *state = *it;
        delete state;
    }
    set<AbstractState *>().swap(states);
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
    assert(nexts == prevs);

    int facts = 0;
    for (int var = 0; var < g_variable_domain.size(); ++var) {
        facts += g_variable_domain[var];
    }
    // Each bitset takes about 32 B.
    int bitset_bytes_single = get_num_states() * ((facts / 8) + 32);

    cout << "Next-arcs: " << nexts << endl;
    cout << "Prev-arcs: " << prevs << endl;
    cout << "Self-loops: " << total_loops << endl;
    cout << "Arcs total: " << nexts + prevs + total_loops << endl;
    cout << "Deviations: " << deviations << endl;
    cout << "Unmet preconditions: " << unmet_preconditions << endl;
    cout << "Unmet goals: " << unmet_goals << endl;
    cout << "Unreachable states: " << unreachable_states << endl;
    cout << "Bitset size single: " << bitset_bytes_single / 1024 << " KB" << endl;
    cout << "Arc size: " << arc_size / 1024 << " KB" << endl;
    cout << "Init h: " << init->get_h() << endl;
    cout << "Average h: " << get_avg_h() << endl;
}
}

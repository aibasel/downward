#include "abstraction.h"

#include "abstract_state.h"
#include "utils.h"
#include "../globals.h"
#include "../operator.h"
#include "../rng.h"
#include "../successor_generator.h"
#include "../timer.h"
#include "../utilities.h"

#include "../ext/btree/btree_map.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <queue>
#include <sstream>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace std;

namespace cegar_heuristic {
typedef btree::btree_map<AbstractState *, Splits> StatesToSplits;

Abstraction::Abstraction()
    : single(new AbstractState()),
      init(single),
      goal(single),
      open(new AdaptiveQueue<AbstractState *>()),
      pick(RANDOM),
      rng(2012),
      needed_operator_costs(),
      num_states(1),
      deviations(0),
      unmet_preconditions(0),
      unmet_goals(0),
      num_states_offline(-1),
      last_avg_h(0),
      last_init_h(0),
      max_states_offline(1),
      max_states_online(0),
      max_time(INF),
      log_h(false),
      calculate_needed_operator_costs(false),
      memory_released(false) {
    assert(!g_goal.empty());

    assert(!g_memory_padding);
    cout << "Reserving " << g_memory_padding_mb << " MB of memory padding." << endl;
    g_memory_padding = new char [g_memory_padding_mb * 1024 * 1024];

    split_tree.set_root(single);
    for (int i = 0; i < g_operators.size(); ++i) {
        single->add_loop(&g_operators[i]);
    }
    states.insert(init);
    if (DEBUG) {
        cout << "Variable domain sizes: ";
        for (int var = 0; var < g_variable_domain.size(); ++var) {
            cout << var << ":" << g_variable_domain[var] << " ";
        }
        cout << endl;
        for (int i = 0; i < g_original_goal.size(); ++i) {
            int var = g_original_goal[i].first;
            int value = g_original_goal[i].second;
            cout << "Goal " << var << "=" << value << " " << g_fact_names[var][value] << endl;
        }
    }
}

Abstraction::~Abstraction() {
    if (!memory_released)
        release_memory();
}

void Abstraction::build() {
    if (WRITE_DOT_FILES) {
        write_causal_graph(*g_causal_graph);
        assert(get_num_states() == 1);
        write_dot_file(get_num_states());
    }
    // Define here to use it outside the loop later.
    bool valid_conc_solution = false;
    while (may_keep_refining()) {
        bool solution_found = find_solution(init);
        if (!solution_found) {
            cout << "Abstract problem is unsolvable!" << endl;
            exit(0);
        }
        valid_conc_solution = check_and_break_solution(*g_initial_state, init);
        if (valid_conc_solution)
            break;
    }
    // Remember how many states were refined offline.
    num_states_offline = get_num_states();
    assert(num_states_offline == states.size());
    cout << "Done building abstraction [t=" << g_timer << "]" << endl;
    cout << "Peak memory after building abstraction: "
         << get_peak_memory_in_kb() << " KB" << endl;
    cout << "Solution found while refining: " << valid_conc_solution << endl;
    cout << "Abstract states offline: " << num_states_offline << endl;

    // Even if we found a valid concrete solution, we might have refined in the
    // last iteration, so we must update the h-values.
    update_h_values();
    log_h_values();
}

void Abstraction::break_solution(AbstractState *state, const Splits &splits) {
    // Log h-values while they are up-to-date.
    log_h_values();
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
    refine(state, var, wanted);
}

void Abstraction::refine(AbstractState *state, int var, const vector<int> &wanted) {
    assert(may_keep_refining());
    if (DEBUG)
        cout << "Refine " << state->str() << " for "
             << var << "=" << to_string(wanted) << endl;
    AbstractState *v1 = new AbstractState();
    AbstractState *v2 = new AbstractState();
    state->split(var, wanted, v1, v2);

    states.erase(state);
    states.insert(v1);
    states.insert(v2);
    ++num_states;

    // Since the search may be started from arbitrary states during online
    // refinements, we can't assume v2 is never "init" and v1 is never "goal".
    if (state == init) {
        if (v1->is_abstraction_of(*g_initial_state)) {
            assert(!v2->is_abstraction_of(*g_initial_state));
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
        cout << "Abstract states: " << num_states << "/"
             << max_states_offline + max_states_online << endl;
    if (WRITE_DOT_FILES)
        write_dot_file(num_states);
}

AbstractState *Abstraction::improve_h(const State &state, AbstractState *abs_state) {
    int rounds = 0;
    const int old_h = abs_state->get_h();
    // Loop until the heuristic value increases.
    while (abs_state->get_h() == old_h && may_keep_refining()) {
        find_solution();
        if (abs_state->get_h() == INF) {
            cout << "No abstract solution starts at " << abs_state->str() << endl;
            break;
        }
        bool solution_valid = check_and_break_solution(*g_initial_state, init);
        if (solution_valid) {
            cout << "Concrete solution found" << endl;
            break;
        }
        abs_state = get_abstract_state(state);
        ++rounds;
    }
    assert(abs_state->get_h() >= old_h);
    if (DEBUG)
        cout << "Refinement rounds: " << rounds << endl;
    return abs_state;
}

void Abstraction::reset_distances_and_solution() const {
    set<AbstractState *>::iterator it;
    for (it = states.begin(); it != states.end(); ++it) {
        AbstractState *state = (*it);
        state->set_distance(INF);
    }
}

bool Abstraction::breadth_first_search(queue<AbstractState *> &open_queue, bool forward) const {
    assert(g_is_unit_cost);
    while (!open_queue.empty()) {
        AbstractState *state = open_queue.front();
        open_queue.pop();
        const int g = state->get_distance();
        if (DEBUG)
            cout << endl << "Expand: " << state->str() << " g:" << g << endl;
        StatesToOps &successors = (forward) ? state->get_arcs_out() : state->get_arcs_in();
        for (StatesToOps::iterator it = successors.begin(); it != successors.end(); ++it) {
            AbstractState *successor = it->first;
            const int succ_g = g + 1;
            if (succ_g < successor->get_distance()) {
                if (DEBUG)
                    cout << "  Succ: " << successor->str()
                         << " g:" << succ_g
                         << " dist:" << successor->get_distance() << endl;
                successor->set_distance(succ_g);
                open_queue.push(successor);
            }
        }
    }
    if ((forward && goal->get_distance() == INF) ||
        (!forward && init->get_distance() == INF)) {
        return false;
    }
    return true;
}

bool Abstraction::dijkstra_search(bool forward) const {
    while (!open->empty()) {
        pair<int, AbstractState *> top_pair = open->pop();
        int &old_g = top_pair.first;
        AbstractState *state = top_pair.second;

        const int g = state->get_distance();
        assert(g < INF);
        assert(g <= old_g);
        if (g < old_g) {
            continue;
        }
        if (DEBUG)
            cout << endl << "Expand: " << state->str() << " g:" << g << endl;
        StatesToOps &successors = (forward) ? state->get_arcs_out() : state->get_arcs_in();
        for (StatesToOps::iterator it = successors.begin(); it != successors.end(); ++it) {
            AbstractState *successor = it->first;
            Operators &ops = it->second;
            assert(!ops.empty());
            for (int i = 0; i < ops.size(); ++i) {
                Operator *op = ops[i];

                // Special code for additive abstractions.
                if (calculate_needed_operator_costs) {
                    assert(forward);
                    // We are currently collecting the needed operator costs.
                    assert(needed_operator_costs.size() == g_operators.size());
                    // cost'(op) = h(a1) - h(a2)
                    const int needed_costs = state->get_h() - successor->get_h();
                    if (needed_costs > 0) {
                        // needed_costs is negative if we reach a2 with op and
                        // h(a2) > h(a1). This includes the case when we reach a
                        // dead-end node. If h(a1)==h(a2) we don't have to update
                        // anything since we initialize the list with zeros. This
                        // handles moving from one dead-end node to another.
                        const int op_index = get_op_index(op);
                        needed_operator_costs[op_index] = max(needed_operator_costs[op_index], needed_costs);
                    }
                }

                const int succ_g = g + op->get_cost();
                assert(succ_g >= 0);

                // If we use Dijkstra instead of A*, we can use "<" here instead of "<=".
                // This leads to way fewer queue pushes.
                if (succ_g < successor->get_distance()) {
                    if (DEBUG)
                        cout << "  Succ: " << successor->str()
                             << " g:" << succ_g
                             << " dist:" << successor->get_distance() << endl;
                    successor->set_distance(succ_g);
                    open->push(succ_g, successor);
                }
                // All subsequent operators don't have lower cost and thus cannot
                // improve the distance. This check reduces the mean runtime by ~8%.
                if (op->get_cost() == g_min_action_cost)
                    break;
            }
        }
    }
    if ((forward && goal->get_distance() == INF) ||
        (!forward && init->get_distance() == INF)) {
        return false;
    }
    return true;
}

bool Abstraction::find_solution(AbstractState *start) {
    if (!start)
        start = init;

    // If we updated the g-values first, they would be overwritten during the
    // computation of the h-values.
    update_h_values();

    reset_distances_and_solution();
    start->set_distance(0);

    if (g_is_unit_cost) {
        queue<AbstractState *> open_queue;
        open_queue.push(start);
        return breadth_first_search(open_queue, true);
    }
    open->clear();
    open->push(0, start);
    return dijkstra_search(true);
}

bool Abstraction::check_and_break_solution(State conc_state, AbstractState *abs_state) {
    if (!abs_state)
        abs_state = get_abstract_state(conc_state);
    assert(abs_state->is_abstraction_of(conc_state));

    if (DEBUG)
        cout << "Check solution." << endl << "Start at       " << abs_state->str()
             << " (is init: " << (abs_state == init) << ")" << endl;

    StatesToSplits states_to_splits;
    queue<pair<AbstractState *, State> > unseen;
    unordered_set<State, hash_state> seen;

    unseen.push(make_pair(abs_state, conc_state));

    int h_0 = init->get_h();
    assert(h_0 != INF);

    while (!unseen.empty()) {
        abs_state = unseen.front().first;
        conc_state = unseen.front().second;
        unseen.pop();
        if (DEBUG)
            cout << "Current state: " << abs_state->str() << endl;
        // Start check from each state only once.
        if (!states_to_splits[abs_state].empty())
            continue;
        int g = abs_state->get_distance();
        assert((g + abs_state->get_h() == h_0) && "State must be on optimal path.");
        if (abs_state == goal) {
            if (test_cegar_goal(conc_state)) {
                // We found a valid concrete solution.
                return true;
            } else {
                // Get unmet goals and refine the last state.
                if (DEBUG)
                    cout << "      Goal test failed." << endl;
                unmet_goals++;
                get_unmet_goal_conditions(conc_state, &states_to_splits[abs_state]);
                continue;
            }
        }
        StatesToOps &arcs_out = abs_state->get_arcs_out();
        for (StatesToOps::iterator it = arcs_out.begin(); it != arcs_out.end(); ++it) {
            AbstractState *next_abs = it->first;
            int next_g = next_abs->get_distance();
            if (next_g + next_abs->get_h() != h_0)
                // State is not on optimal path.
                continue;
            Operators &ops = it->second;
            assert(!ops.empty());
            for (int i = 0; i < ops.size(); ++i) {
                Operator *op = ops[i];
                if (g + op->get_cost() != next_g)
                    // Operator is not part of an optimal path.
                    continue;
                if (op->is_applicable(conc_state)) {
                    if (DEBUG)
                        cout << "      Move to: " << next_abs->str()
                             << " with " << op->get_name() << endl;
                    State next_conc = State(conc_state, *op);
                    if (next_abs->is_abstraction_of(next_conc)) {
                        if (seen.count(next_conc) == 0) {
                            unseen.push(make_pair(next_abs, next_conc));
                            seen.insert(next_conc);
                        }
                    // Only find deviation reasons if we haven't found any splits already.
                    } else if (states_to_splits[abs_state].empty()) {
                        if (DEBUG)
                            cout << "      Paths deviate." << endl;
                        ++deviations;
                        AbstractState desired_abs_state;
                        next_abs->regress(*op, &desired_abs_state);
                        abs_state->get_possible_splits(desired_abs_state, conc_state,
                                                       &states_to_splits[abs_state]);
                    }
                // Only find unmet preconditions if we haven't found any splits already.
                } else if (states_to_splits[abs_state].empty()) {
                    if (DEBUG)
                        cout << "      Operator not applicable: " << op->get_name() << endl;
                    ++unmet_preconditions;
                    get_unmet_preconditions(*op, conc_state, &states_to_splits[abs_state]);
                }
                // We only need one arc between two states.
                break;
            }
        }
    }
    int broken_solutions = 0;
    for (StatesToSplits::iterator it = states_to_splits.begin(); it != states_to_splits.end(); ++it) {
        if (!may_keep_refining())
            break;
        AbstractState *state = it->first;
        Splits &splits = it->second;
        if (!splits.empty()) {
            break_solution(state, splits);
            ++broken_solutions;
        }
    }
    if (DEBUG)
        cout << "Broke " << broken_solutions << " solutions" << endl;
    assert(broken_solutions > 0);
    return false;
}

int Abstraction::pick_split_index(AbstractState &state, const Splits &splits) const {
    assert(!splits.empty());
    // Shortcut for condition lists with only one element.
    if (splits.size() == 1) {
        return 0;
    }
    if (DEBUG) {
        cout << "Split: " << state.str() << endl;
        // TODO: Handle overlapping splits.
        for (int i = 0; i < splits.size(); ++i) {
            const Split &split = splits[i];
            int var = split.first;
            const vector<int> &wanted = split.second;
            if (DEBUG)
                cout << var << "=" << to_string(wanted) << " ";
        }
    }
    if (DEBUG)
        cout << endl;
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
        int min_rest = INF;
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
            double refinement = -(rest / double(all_values));
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
        int min_pos = INF;
        int max_pos = -1;
        for (int i = 0; i < splits.size(); ++i) {
            int var = splits[i].first;
            int pos = g_causal_graph_ordering_pos[var];
            assert(pos != UNDEFINED);
            if (pick == MIN_PREDECESSORS && pos < min_pos) {
                cond = i;
                min_pos = pos;
            }
            if (pick == MAX_PREDECESSORS && pos > max_pos) {
                cond = i;
                max_pos = pos;
            }
        }
    } else if (pick == MIN_OPS || pick == MAX_OPS) {
        // TODO: Rewrite or leave out commented out code.
        cond = 0;
        /*
        // Make the variables easily accessible.
        vector<int> vars(splits.size());
        for (int i = 0; i < splits.size(); ++i) {
            vars[i] = splits[i].first;
        }

        // Record number of new ops per variable.
        vector<int> new_ops(vars.size(), 0);

        // Loop over all incoming ops o_in and record for each possible split
        // variable the number of resulting operators. For each operator we get
        // one new operator if eff(o_in) is defined and two otherwise.
        for (int i = 0; i < state.get_prev().size(); ++i) {
            Operator *op = state.get_prev()[i].first;
            for (int j = 0; j < vars.size(); ++j) {
                int eff = get_post(*op, vars[j]);
                new_ops[j] += (eff == UNDEFINED) ? 2 : 1;
            }
        }
        // Loop over all outgoing ops o_out and record for each possible split
        // variable the number of resulting operators. If eff(o_in) is defined
        // we get one new operator, else two.
        for (int i = 0; i < state.get_next().size(); ++i) {
            Operator *op = state.get_next()[i].first;
            for (int j = 0; j < vars.size(); ++j) {
                int pre = get_pre(*op, vars[j]);
                new_ops[j] += (pre == UNDEFINED) ? 2 : 1;
            }
        }
        // Loop over all self-loops o and record for each possible split
        // variable the number of resulting operators. If pre(o) is defined we
        // get one new operator, else two.
        for (int i = 0; i < state.get_loops().size(); ++i) {
            Operator *op = state.get_loops()[i];
            for (int j = 0; j < vars.size(); ++j) {
                int pre = get_pre(*op, vars[j]);
                new_ops[j] += (pre == UNDEFINED) ? 2 : 1;
            }
        }
        cout << "Tentative new ops: " << to_string(new_ops) << endl;
        if (pick == MIN_OPS) {
            cond = min_element(new_ops.begin(), new_ops.end()) - new_ops.begin();
        } else {
            cond = max_element(new_ops.begin(), new_ops.end()) - new_ops.begin();
        }
        */
    } else {
        cout << "Invalid pick strategy: " << pick << endl;
        exit(2);
    }
    assert(cond >= 0);
    return cond;
}

void Abstraction::update_h_values() const {
    reset_distances_and_solution();
    goal->set_distance(0);
    if (g_is_unit_cost) {
        queue<AbstractState *> open_queue;
        open_queue.push(goal);
        breadth_first_search(open_queue, false);
    } else {
        open->clear();
        open->push(0, goal);
        dijkstra_search(false);
    }
    set<AbstractState *>::iterator it;
    for (it = states.begin(); it != states.end(); ++it) {
        AbstractState *state = *it;
        state->set_h(state->get_distance());
    }
}

double Abstraction::get_avg_h() const {
    // This function is called after the abstraction has been built, so all
    // h-values are up-to-date.
    double avg_h = 0.;
    set<AbstractState *>::iterator it;
    for (it = states.begin(); it != states.end(); ++it) {
        AbstractState *state = *it;
        const int h = state->get_h();
        assert(h >= 0);
        if (h != INF) {
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
        StatesToOps &next = current_state->get_arcs_out();
        for (StatesToOps::iterator it = next.begin(); it != next.end(); ++it) {
            AbstractState *next_state = it->first;
            Operators &ops = it->second;
            for (int i = 0; i < ops.size(); ++i) {
                Operator *op = ops[i];
                dotfile << current_state->str() << " -> " << next_state->str()
                        << " [label=\"" << op->get_name() << "\"];" << endl;
            }
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

int Abstraction::get_op_index(const Operator *op) const {
    int op_index = op - &*g_operators.begin();
    assert(op_index >= 0 && op_index < g_operators.size());
    return op_index;
}

void Abstraction::adapt_operator_costs() {
    needed_operator_costs.resize(g_operators.size(), 0);
    // Traverse abstraction and remember the minimum cost we need to keep for
    // each operator in order not to decrease any heuristic values.
    open->clear();
    reset_distances_and_solution();
    init->set_distance(0);
    open->push(0, init);
    calculate_needed_operator_costs = true;
    dijkstra_search(true);
    calculate_needed_operator_costs = false;
    for (int i = 0; i < needed_operator_costs.size(); ++i) {
        if (DEBUG)
            cout << i << " " << needed_operator_costs[i] << "/"
                 << g_operators[i].get_cost() << " " << g_operators[i].get_name() << endl;
        Operator &op = g_operators[i];
        op.set_cost(op.get_cost() - needed_operator_costs[i]);
        assert(op.get_cost() >= 0);
    }
}

int Abstraction::get_num_states_online() const {
    assert(num_states_offline >= 1);
    return get_num_states() - num_states_offline;
}

bool Abstraction::may_keep_refining() const {
    return g_memory_padding &&
           (is_online() || get_num_states() < max_states_offline) &&
           (!is_online() || get_num_states_online() < max_states_online) &&
           (max_time == INF || is_online() || timer() < max_time);
}

void Abstraction::release_memory() {
    cout << "Release memory" << endl;
    assert(!memory_released);
    delete open;
    open = 0;
    if (g_memory_padding) {
        delete[] g_memory_padding;
        g_memory_padding = 0;
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
    // TODO: Rewrite or leave out commented out code.
    //int nexts = 0, prevs = 0, total_loops = 0;
    int unreachable_states = 0;
    //int arc_size = 0;
    set<AbstractState *>::iterator it;
    for (it = states.begin(); it != states.end(); ++it) {
        AbstractState *state = *it;
        if (state->get_h() == INF)
            ++unreachable_states;
        //Arcs &next = state->get_next();
        //Arcs &prev = state->get_prev();
        //Loops &loops = state->get_loops();
        //nexts += next.size();
        //prevs += prev.size();
        //total_loops += loops.size();
        //arc_size += sizeof(next) + sizeof(Arc) * next.capacity() +
        //            sizeof(prev) + sizeof(Arc) * prev.capacity() +
        //            sizeof(loops) + sizeof(Operator *) * loops.capacity();
    }
    //assert(nexts == prevs);

    //cout << "Next-arcs: " << nexts << endl;
    //cout << "Prev-arcs: " << prevs << endl;
    //cout << "Self-loops: " << total_loops << endl;
    //cout << "Arcs total: " << nexts + prevs + total_loops << endl;
    cout << "Deviations: " << deviations << endl;
    cout << "Unmet preconditions: " << unmet_preconditions << endl;
    cout << "Unmet goals: " << unmet_goals << endl;
    cout << "Unreachable states: " << unreachable_states << endl;
    //cout << "Arc size: " << arc_size / 1024 << " KB" << endl;
    cout << "Init h: " << init->get_h() << endl;
    cout << "Average h: " << get_avg_h() << endl;
    cout << "CEGAR abstractions: 1" << endl;
}
}

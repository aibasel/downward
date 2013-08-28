#include "abstraction.h"

#include "abstract_state.h"
#include "utils.h"
#include "../additive_heuristic.h"
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
#include <new>
#include <queue>
#include <sstream>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace std;

namespace cegar_heuristic {
typedef btree::btree_map<AbstractState *, Splits> StatesToSplits;

static char *cegar_memory_padding = 0;

void no_memory_continue () {
    assert(cegar_memory_padding);
    delete[] cegar_memory_padding;
    cegar_memory_padding = 0;
    cout << "Failed to allocate memory for CEGAR abstraction. "
         << "Released memory padding and will stop refinement now." << endl;
    set_new_handler(no_memory);
}

Abstraction::Abstraction()
    : single(new AbstractState()),
      init(single),
      goal(single),
      open(new AdaptiveQueue<AbstractState *>()),
      pick(RANDOM),
      rng(2012),
      needed_operator_costs(),
      hadd(0),
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
      max_init_h(INF),
      use_astar(true),
      log_h(false),
      calculate_needed_operator_costs(false),
      memory_released(false) {
    assert(!g_goal.empty());

    assert(!cegar_memory_padding);
    cout << "Reserving " << g_memory_padding_mb << " MB of memory padding." << endl;
    cegar_memory_padding = new char[g_memory_padding_mb * 1024 * 1024];
    set_new_handler(no_memory_continue);

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

void Abstraction::set_pick_strategy(PickStrategy strategy) {
    pick = strategy;
    cout << "Use flaw-selection strategy " << pick << endl;
    if (pick == MIN_LM || pick == MAX_LM ||
        pick == MIN_HADD_MIN_LM || pick == MIN_HADD_MAX_LM ||
        pick == MAX_HADD_MIN_LM || pick == MAX_HADD_MAX_LM) {
        vector<int> ordering;
        order_facts_in_landmark_graph(&ordering);
        fact_positions_in_lm_graph_ordering.resize(g_num_facts, -1);
        for (int i = 0; i < ordering.size(); ++i) {
            int fact_number = ordering[i];
            fact_positions_in_lm_graph_ordering[fact_number] = i;
        }
        if (DEBUG) {
            cout << "Check that landmark graph is ordered by h^add values:" << endl;
            int last_hadd_value = 0;
            for (int i = 0; i < ordering.size(); ++i) {
                int var = -1;
                int value = -1;
                get_fact_from_number(ordering[i], var, value);
                int hadd_value = hadd->get_cost(var, value);
                cout << var << "=" << value << " " << g_fact_names[var][value]
                         << " cost:" << hadd_value
                         << " number:" << ordering[i]
                         << " pos:" << i << endl;
                if (hadd_value < last_hadd_value)
                    cout << "Landmarks are not ordered by h^add values. "
                            "The landmark graph is probably not fully connected." << endl;
                last_hadd_value = hadd_value;
            }
        }
    }
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
        if (use_astar) {
            find_solution();
        } else {
            update_h_values();
        }
        if ((use_astar && goal->get_distance() == INF) ||
            (!use_astar && init->get_h() == INF)) {
            cout << "Abstract problem is unsolvable!" << endl;
            exit_with(EXIT_UNSOLVABLE);
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
    cout << "Single abstraction init-h: " << init->get_h() << endl;
}

void Abstraction::break_solution(AbstractState *state, const Splits &splits) {
    // Log h-values while they are up-to-date.
    log_h_values();
    int avg_h_check_step = 50000;
    if (num_states % avg_h_check_step == 0) {
        double new_avg_h = get_avg_h();
        double progress = (new_avg_h - last_avg_h) * 100000 / avg_h_check_step;
        cout << "Avg-h: " << new_avg_h << ", progress: " << progress << endl;
        last_avg_h = new_avg_h;
    }
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
        update_h_values();
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
    for (auto it = states.begin(); it != states.end(); ++it) {
        AbstractState *state = (*it);
        state->set_distance(INF);
        state->set_prev_solution_op(0);
        state->set_next_solution_op(0);
        state->set_prev_solution_state(0);
        state->set_next_solution_state(0);
    }
}

bool Abstraction::astar_search(bool forward, bool use_h) const {
    bool debug = true && DEBUG;
    while (!open->empty()) {
        pair<int, AbstractState *> top_pair = open->pop();
        int &old_f = top_pair.first;
        AbstractState *state = top_pair.second;

        const int g = state->get_distance();
        assert(g < INF);
        int new_f = g;
        if (use_h)
            new_f += state->get_h();
        assert(new_f <= old_f);
        if (new_f < old_f) {
            continue;
        }
        if (debug)
            cout << endl << "Expand: " << state->str() << " g:" << g
                 << " h:" << state->get_h() << " f:" << new_f << endl;
         if (forward && use_h && state == goal) {
            if (debug)
                cout << "GOAL REACHED" << endl;
            return true;
        }
        Arcs &successors = (forward) ? state->get_arcs_out() : state->get_arcs_in();
        for (auto it = successors.begin(); it != successors.end(); ++it) {
            Operator *op = it->first;
            AbstractState *successor = it->second;

            // Special code for additive abstractions.
            if (calculate_needed_operator_costs) {
                assert(forward);
                assert(!use_h);
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
                if (debug)
                    cout << "  Succ: " << successor->str()
                         << " f:" << succ_g + successor->get_h()
                         << " g:" << succ_g
                         << " dist:" << successor->get_distance()
                         << " h:" << successor->get_h() << endl;

                successor->set_distance(succ_g);
                int f = succ_g;
                if (use_h) {
                    int h = successor->get_h();
                    // Ignore dead-end states.
                    if (h == INF)
                        continue;
                    f += h;
                }
                assert(f >= 0);
                open->push(f, successor);
                successor->set_prev_solution_op(op);
                successor->set_prev_solution_state(state);
            }
        }
    }
    if ((forward && goal->get_distance() == INF) ||
        (!forward && init->get_distance() == INF)) {
        return false;
    }
    return true;
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

    // Only search flaws until we hit the memory limit.
    while (!unseen.empty() && cegar_memory_padding) {
        abs_state = unseen.front().first;
        conc_state = unseen.front().second;
        unseen.pop();
        Splits &splits = states_to_splits[abs_state];
        if (DEBUG)
            cout << "Current state: " << abs_state->str() << endl;
        // Start check from each state only once.
        if (!states_to_splits[abs_state].empty())
            continue;
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
                restrict_splits(conc_state, splits);
                continue;
            }
        }
        Arcs &arcs_out = abs_state->get_arcs_out();
        for (auto it = arcs_out.begin(); it != arcs_out.end(); ++it) {
            Operator *op = it->first;
            AbstractState *next_abs = it->second;
            assert(!use_astar || (abs_state->get_next_solution_op() &&
                                  abs_state->get_next_solution_state()));
            if (use_astar && (op != abs_state->get_next_solution_op() ||
                              next_abs != abs_state->get_next_solution_state()))
                continue;
            if (next_abs->get_h() + op->get_cost() != abs_state->get_h())
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
                } else if (splits.empty()) {
                    if (DEBUG)
                        cout << "      Paths deviate." << endl;
                    ++deviations;
                    AbstractState desired_abs_state;
                    next_abs->regress(*op, &desired_abs_state);
                    abs_state->get_possible_splits(desired_abs_state, conc_state,
                                                   &splits);
                    restrict_splits(conc_state, splits);
                }
            // Only find unmet preconditions if we haven't found any splits already.
            } else if (splits.empty()) {
                if (DEBUG)
                    cout << "      Operator not applicable: " << op->get_name() << endl;
                ++unmet_preconditions;
                get_unmet_preconditions(*op, conc_state, &splits);
                restrict_splits(conc_state, splits);
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
    assert(!use_astar || broken_solutions == 1);
    return false;
}

void Abstraction::restrict_splits(State &conc_state, Splits &splits) const {
    if (splits.size() == 1)
        return;
    if (pick == MIN_HADD_DYN || pick == MAX_HADD_DYN) {
        assert(hadd);
        evaluate_state_with_hadd(conc_state);
        int min_hadd = INF;
        int max_hadd = -1;
        vector<int> incumbents;
        for (int i = 0; i < splits.size(); ++i) {
            int var = splits[i].first;
            const vector<int> &values = splits[i].second;
            for (int j = 0; j < values.size(); ++j) {
                int value = values[j];
                int hadd_value = hadd->get_cost(var, value);
                assert(hadd_value != -1);
                if (pick == MIN_HADD_DYN) {
                    if (hadd_value < min_hadd) {
                        incumbents.clear();
                        min_hadd = hadd_value;
                    }
                    if (hadd_value <= min_hadd)
                        incumbents.push_back(i);
                }
                else {
                    if (hadd_value > max_hadd) {
                        incumbents.clear();
                        max_hadd = hadd_value;
                    }
                    if (hadd_value >= max_hadd)
                        incumbents.push_back(i);
                }
            }
        }
        assert(!incumbents.empty());
        swap(splits[0], splits[incumbents[0]]);
        splits.resize(1);
    }
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
        cond = 0;
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
        for (int i = 0; i < state.get_arcs_in().size(); ++i) {
            Operator *op = state.get_arcs_in()[i].first;
            for (int j = 0; j < vars.size(); ++j) {
                int eff = get_post(*op, vars[j]);
                new_ops[j] += (eff == UNDEFINED) ? 2 : 1;
            }
        }
        // Loop over all outgoing ops o_out and record for each possible split
        // variable the number of resulting operators. If eff(o_in) is defined
        // we get one new operator, else two.
        for (int i = 0; i < state.get_arcs_out().size(); ++i) {
            Operator *op = state.get_arcs_out()[i].first;
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
        if (pick == MIN_OPS) {
            cond = min_element(new_ops.begin(), new_ops.end()) - new_ops.begin();
        } else {
            cond = max_element(new_ops.begin(), new_ops.end()) - new_ops.begin();
        }
    } else if (pick == MIN_LM || pick == MAX_LM) {
        assert(!fact_positions_in_lm_graph_ordering.empty());
        int min = INF;
        int max = -1;
        for (int i = 0; i < splits.size(); ++i) {
            int var = splits[i].first;
            const vector<int> &values = splits[i].second;
            for (int j = 0; j < values.size(); ++j) {
                int value = values[j];
                int fact_number = get_fact_number(var, value);
                int lm_pos = fact_positions_in_lm_graph_ordering[fact_number];
                if (lm_pos == -1)
                    continue;
                if (pick == MIN_LM && lm_pos < min) {
                    cond = i;
                    min = lm_pos;
                }
                if (pick == MAX_LM && lm_pos > max) {
                    cond = i;
                    max = lm_pos;
                }
            }
        }
        if (cond == -1)
            cond = random_cond;
    } else if (pick == MIN_HADD || pick == MAX_HADD ||
               pick == MIN_HADD_MIN_LM || pick == MIN_HADD_MAX_LM ||
               pick == MAX_HADD_MIN_LM || pick == MAX_HADD_MAX_LM) {
        assert(hadd);
        bool pick_min_hadd = (pick == MIN_HADD) || (pick == MIN_HADD_MIN_LM) ||
                             (pick == MIN_HADD_MAX_LM);
        int min_hadd = INF;
        int max_hadd = -1;
        vector<int> incumbents;
        for (int i = 0; i < splits.size(); ++i) {
            int var = splits[i].first;
            const vector<int> &values = splits[i].second;
            for (int j = 0; j < values.size(); ++j) {
                int value = values[j];
                int hadd_value = hadd->get_cost(var, value);
                assert(hadd_value != -1);
                if (pick_min_hadd) {
                    if (hadd_value < min_hadd) {
                        incumbents.clear();
                        min_hadd = hadd_value;
                    }
                    if (hadd_value <= min_hadd)
                        incumbents.push_back(i);
                }
                else {
                    if (hadd_value > max_hadd) {
                        incumbents.clear();
                        max_hadd = hadd_value;
                    }
                    if (hadd_value >= max_hadd)
                        incumbents.push_back(i);
                }
            }
        }
        if (DEBUG)
            cout << "Incumbents: " << incumbents << endl;
        assert(!incumbents.empty());
        if (pick == MIN_HADD || pick == MAX_HADD)
            return incumbents[0];

        assert(!fact_positions_in_lm_graph_ordering.empty());
        bool pick_min_lm = (pick == MIN_HADD_MIN_LM) || (pick == MAX_HADD_MIN_LM);
        int min = INF;
        int max = -1;
        for (int i = 0; i < incumbents.size(); ++i) {
            const Split &split = splits[incumbents[i]];
            int var = split.first;
            const vector<int> &values = split.second;
            for (int j = 0; j < values.size(); ++j) {
                int value = values[j];
                int fact_number = get_fact_number(var, value);
                int lm_pos = fact_positions_in_lm_graph_ordering[fact_number];
                if (lm_pos == -1) {
                    // If any incumbent is missing in the landmark ordering, we
                    // don't use the ordering.
                    cond = incumbents[0];
                    break;
                }
                if (pick_min_lm && lm_pos < min) {
                    cond = incumbents[i];
                    min = lm_pos;
                } else if (!pick_min_lm && lm_pos > max) {
                    cond = incumbents[i];
                    max = lm_pos;
                }
            }
        }
    } else if (pick == MIN_HADD_DYN || pick == MAX_HADD_DYN) {
        // The selection has already been made.
    } else {
        cout << "Invalid pick strategy: " << pick << endl;
        exit_with(EXIT_INPUT_ERROR);
    }
    assert(cond >= 0);
    assert(cond < splits.size());
    return cond;
}

void Abstraction::extract_solution() const {
    assert(goal->get_distance() != INF);
    assert(goal->get_h() == 0);
    AbstractState *current = goal;
    while (current != init) {
        if (DEBUG)
            cout << endl << "Current: " << current->str() << " g:" << current->get_distance()
                 << " h:" << current->get_h() << endl;
        Operator *prev_op = current->get_prev_solution_op();
        AbstractState *prev_state = current->get_prev_solution_state();
        prev_state->set_next_solution_op(prev_op);
        prev_state->set_next_solution_state(current);
        prev_state->set_h(current->get_h() + prev_op->get_cost());
        assert(prev_state != current);
        current = prev_state;
    }
    assert(init->get_h() == goal->get_distance());
}

void Abstraction::find_solution() const {
    reset_distances_and_solution();
    open->clear();
    init->set_distance(0);
    open->push(init->get_h(), init);
    bool success = astar_search(true, true);
    if (success)
        extract_solution();
}

void Abstraction::update_h_values() const {
    reset_distances_and_solution();
    open->clear();
    goal->set_distance(0);
    open->push(0, goal);
    astar_search(false, false);
    for (auto it = states.begin(); it != states.end(); ++it) {
        AbstractState *state = *it;
        state->set_h(state->get_distance());
    }
}

double Abstraction::get_avg_h() const {
    // This function is called after the abstraction has been built, so all
    // h-values are up-to-date.
    double avg_h = 0.;
    for (auto it = states.begin(); it != states.end(); ++it) {
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
        exit_with(EXIT_CRITICAL_ERROR);
    }
    dotfile << "digraph abstract {" << endl;
    AbstractStates::iterator it;
    for (it = states.begin(); it != states.end(); ++it) {
        AbstractState *current_state = *it;
        Arcs &next = current_state->get_arcs_out();
        for (auto it = next.begin(); it != next.end(); ++it) {
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
    astar_search(true, false);
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
    return cegar_memory_padding &&
           (is_online() || get_num_states() < max_states_offline) &&
           (!is_online() || get_num_states_online() < max_states_online) &&
           (max_time == INF || is_online() || timer() < max_time) &&
           (init->get_h() < max_init_h);
}

void Abstraction::set_hadd(AdditiveHeuristic *h) {
    hadd = h;
    evaluate_state_with_hadd(*g_initial_state);
}

void Abstraction::set_max_init_h_factor(double factor) {
    if (factor == -1) {
        max_init_h = INF;
    } else {
        assert(factor >= 0);
        max_init_h = factor * hadd->get_heuristic();
        cout << "h^add(s_0)=" << hadd->get_heuristic() << endl;
        assert(max_init_h >= 0);
    }
    cout << "Set max_init_h to " << max_init_h << endl;
}

void Abstraction::evaluate_state_with_hadd(State &conc_state) const {
    cout << "Start computing h^add values [t=" << g_timer << "]" << endl;
    hadd->evaluate(conc_state);
    if (DEBUG) {
        cout << "h^add values for all facts:" << endl;
        for (int var = 0; var < g_variable_domain.size(); ++var) {
            for (int value = 0; value < g_variable_domain[var]; ++value) {
                cout << "  " << var << "=" << value << " " << g_fact_names[var][value]
                     << " cost:" << hadd->get_cost(var, value) << endl;
            }
        }
        cout << endl;
    }
    cout << "Done computing h^add values [t=" << g_timer << "]" << endl;
}

void Abstraction::release_memory() {
    cout << "Release memory" << endl;
    assert(!memory_released);
    delete open;
    open = 0;
    if (cegar_memory_padding) {
        delete[] cegar_memory_padding;
        cegar_memory_padding = 0;
    }
    AbstractStates::iterator it;
    for (it = states.begin(); it != states.end(); ++it) {
        AbstractState *state = *it;
        delete state;
    }
    AbstractStates().swap(states);
    delete hadd;
    hadd = 0;
    memory_released = true;
}

void Abstraction::print_statistics() {
    int nexts = 0, prevs = 0, total_loops = 0;
    int unreachable_states = 0;
    int arc_size = 0;
    for (auto it = states.begin(); it != states.end(); ++it) {
        AbstractState *state = *it;
        if (state->get_h() == INF)
            ++unreachable_states;
        Arcs &next = state->get_arcs_out();
        Arcs &prev = state->get_arcs_in();
        Loops &loops = state->get_loops();
        nexts += next.size();
        prevs += prev.size();
        total_loops += loops.size();
        arc_size += sizeof(next) + sizeof(Arc) * next.capacity() +
                    sizeof(prev) + sizeof(Arc) * prev.capacity() +
                    sizeof(loops) + sizeof(Operator *) * loops.capacity();
    }
    assert(nexts == prevs);

    cout << "Next-arcs: " << nexts << endl;
    cout << "Prev-arcs: " << prevs << endl;
    cout << "Self-loops: " << total_loops << endl;
    cout << "Arcs total: " << nexts + prevs + total_loops << endl;
    cout << "Deviations: " << deviations << endl;
    cout << "Unmet preconditions: " << unmet_preconditions << endl;
    cout << "Unmet goals: " << unmet_goals << endl;
    cout << "Unreachable states: " << unreachable_states << endl;
    cout << "Arc size: " << arc_size / 1024 << " KB" << endl;
    cout << "Init h: " << init->get_h() << endl;
    cout << "Average h: " << get_avg_h() << endl;
    cout << "CEGAR abstractions: 1" << endl;
}
}

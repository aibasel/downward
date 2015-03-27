#include "abstraction.h"

#include "abstract_state.h"
#include "landmark_task.h"
#include "utils.h"

#include "../additive_heuristic.h"
#include "../global_operator.h"
#include "../globals.h"
#include "../rng.h"
#include "../state_registry.h"
#include "../successor_generator.h"
#include "../timer.h"
#include "../utilities.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <new>
#include <queue>
#include <set>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;

namespace cegar {
typedef unordered_map<AbstractState *, Splits> StatesToSplits;

static StateRegistry *get_state_registry(const vector<int> &initial_state_data) {
    vector<int> original_initial_state_data;
    original_initial_state_data.swap(g_initial_state_data);
    g_initial_state_data = initial_state_data;
    StateRegistry *registry = new StateRegistry();
    original_initial_state_data.swap(g_initial_state_data);
    return registry;
}

Abstraction::Abstraction(const LandmarkTask *task)
    : task(task),
      task_proxy(TaskProxy(task)),
      registry(get_state_registry(task->get_initial_state_data())),
      single(new AbstractState(task_proxy)),
      init(single),
      open(new AdaptiveQueue<AbstractState *>()),
      pick(RANDOM),
      rng(2012),
      num_states(1),
      deviations(0),
      unmet_preconditions(0),
      unmet_goals(0),
      max_states(1),
      max_time(INF),
      use_astar(true),
      use_general_costs(false),
      dump_graphs(false),
      memory_released(false) {
    assert(!task_proxy.get_goals().empty());
    reserve_memory_padding();

    goals.insert(init);

    split_tree.set_root(single);
    for (OperatorProxy op : task_proxy.get_operators()) {
        const GlobalOperator *global_op = op.get_global_operator();
        single->add_loop(global_op);
        op_to_index[global_op] = op.get_id();
    }
    states.insert(init);
}

Abstraction::~Abstraction() {
    if (!memory_released)
        release_memory();
}

void Abstraction::set_pick_strategy(PickStrategy strategy) {
    pick = strategy;
    cout << "Use flaw-selection strategy " << pick << endl;
}

int Abstraction::get_min_goal_distance() const {
    assert(!memory_released);
    int min_distance = INF;
    for (AbstractStates::const_iterator it = goals.begin(); it != goals.end(); ++it) {
        min_distance = min(min_distance, (*it)->get_distance());
    }
    return min_distance;
}

bool Abstraction::is_goal(AbstractState *state) const {
    return goals.find(state) != goals.end();
}

void Abstraction::separate_unreachable_facts() {
    assert(init == single);
    for (VariableProxy var : task_proxy.get_variables()) {
        int var_id  = var.get_id();
        const unordered_set<int> &unreachable_facts = task->get_unreachable_facts()[var_id];
        // TODO: Use vector<int> directly?
        vector<int> wanted(unreachable_facts.begin(), unreachable_facts.end());
        if (!wanted.empty())
            refine(init, var_id, wanted);
    }
    goals.insert(states.begin(), states.end());
}

void Abstraction::build() {
    if (dump_graphs) {
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
        if ((use_astar && get_min_goal_distance() == INF) ||
            (!use_astar && init->get_h() == INF)) {
            cout << "Abstract problem is unsolvable!" << endl;
            valid_conc_solution = false;
            break;
        }
        valid_conc_solution = check_and_break_solution(registry->get_initial_state(), init);
        if (valid_conc_solution)
            break;
    }
    cout << "Solution found while refining: " << valid_conc_solution << endl;
    // Even if we found a valid concrete solution, we might have refined in the
    // last iteration, so we must update the h-values.
    update_h_values();
    // Remember number of states before we release the memory.
    num_states = get_num_states();
    assert(num_states == static_cast<int>(states.size()));
    print_statistics();
}

void Abstraction::break_solution(AbstractState *state, const Splits &splits) {
    if (DEBUG) {
        cout << "Unmet conditions: ";
        for (size_t i = 0; i < splits.size(); ++i) {
            cout << splits[i].first << "=" << to_string(splits[i].second) << " ";
        }
        cout << endl;
    }
    int i = pick_split_index(*state, splits);
    assert(in_bounds(i, splits));
    int var = splits[i].first;
    const vector<int> &wanted = splits[i].second;
    refine(state, var, wanted);
}

void Abstraction::refine(AbstractState *state, int var, const vector<int> &wanted) {
    assert(may_keep_refining());
    if (DEBUG)
        cout << "Refine " << state->str() << " for "
             << var << "=" << to_string(wanted) << endl;
    AbstractState *v1 = new AbstractState(task_proxy);
    AbstractState *v2 = new AbstractState(task_proxy);
    state->split(var, wanted, v1, v2);

    states.erase(state);
    states.insert(v1);
    states.insert(v2);
    ++num_states;

    // Since the search is always started from the abstract
    // initial state, v2 is never "init" and v1 is never "goal".
    if (state == init) {
        assert(v1->is_abstraction_of(registry->get_initial_state()));
        assert(!v2->is_abstraction_of(registry->get_initial_state()));
        init = v1;
        if (DEBUG)
            cout << "Using new init state: " << init->str() << endl;
    }
    if (is_goal(state)) {
        goals.erase(state);
        goals.insert(v2);
        if (DEBUG)
            cout << "Using new/additional goal state: " << v2->str() << endl;
    }

    int num_states = get_num_states();
    if (num_states % STATES_LOG_STEP == 0)
        cout << "Abstract states: " << num_states << "/"
             << max_states << endl;
    if (dump_graphs)
        write_dot_file(num_states);

    delete state;
}

void Abstraction::reset_distances_and_solution() const {
    for (AbstractStates::const_iterator it = states.begin(); it != states.end(); ++it) {
        AbstractState *state = (*it);
        state->set_distance(INF);
        state->set_prev_solution_op(0);
        state->set_next_solution_op(0);
        state->set_prev_solution_state(0);
        state->set_next_solution_state(0);
    }
}

bool Abstraction::astar_search(bool forward, bool use_h, vector<int> *needed_costs) const {
    bool debug = true && DEBUG;
    while (!open->empty()) {
        pair<int, AbstractState *> top_pair = open->pop();
        int &old_f = top_pair.first;
        AbstractState *state = top_pair.second;

        const int g = state->get_distance();
        assert(0 <= g && g < INF);
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
        if (forward && use_h && is_goal(state)) {
            if (debug)
                cout << "GOAL REACHED" << endl;
            extract_solution(state);
            return true;
        }
        // All operators that induce self-loops need at least 0 costs.
        if (needed_costs) {
            assert(forward);
            assert(!use_h);
            assert(needed_costs->size() == task_proxy.get_operators().size());
            for (size_t i = 0; i < state->get_loops().size(); ++i) {
                const GlobalOperator *op = state->get_loops()[i];
                const int op_index = get_op_index(op);
                (*needed_costs)[op_index] = max((*needed_costs)[op_index], 0);
            }
        }
        Arcs &successors = (forward) ? state->get_arcs_out() : state->get_arcs_in();
        for (Arcs::iterator it = successors.begin(); it != successors.end(); ++it) {
            const GlobalOperator *op = it->first;
            AbstractState *successor = it->second;

            // Collect needed operator costs for additive abstractions.
            if (needed_costs) {
                assert(forward);
                assert(!use_h);
                assert(needed_costs->size() == task_proxy.get_operators().size());
                int needed = state->get_h() - successor->get_h();
                if (!use_general_costs)
                    needed = max(0, needed);
                const int op_index = get_op_index(op);
                (*needed_costs)[op_index] = max((*needed_costs)[op_index], needed);
            }

            assert(op->get_cost() >= 0);
            int succ_g = g + op->get_cost();
            assert(succ_g >= 0);

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
    // TODO: Remove this and the return value.
    if ((forward && get_min_goal_distance() == INF) ||
        (!forward && init->get_distance() == INF)) {
        return false;
    }
    return true;
}

bool Abstraction::check_and_break_solution(GlobalState conc_state, AbstractState *abs_state) {
    assert(abs_state->is_abstraction_of(conc_state));

    if (DEBUG)
        cout << "Check solution." << endl << "Start at       " << abs_state->str()
             << " (is init: " << (abs_state == init) << ")" << endl;

    StatesToSplits states_to_splits;
    queue<pair<AbstractState *, GlobalState> > unseen;
    unordered_set<StateID, hash_state_id> seen;

    unseen.push(make_pair(abs_state, conc_state));

    // Only search flaws until we hit the memory limit.
    while (!unseen.empty() && memory_padding_is_reserved()) {
        abs_state = unseen.front().first;
        conc_state = unseen.front().second;
        unseen.pop();
        Splits &splits = states_to_splits[abs_state];
        if (DEBUG)
            cout << "Current state: " << abs_state->str() << endl;
        // Start check from each state only once.
        if (!states_to_splits[abs_state].empty())
            continue;
        if (is_goal(abs_state)) {
            if (test_goal(conc_state)) {
                // We found a valid concrete solution.
                return true;
            } else {
                // Get unmet goals and refine the last state.
                if (DEBUG)
                    cout << "      Goal test failed." << endl;
                unmet_goals++;
                get_unmet_goals(task_proxy.get_goals(), conc_state, &states_to_splits[abs_state]);
                continue;
            }
        }
        Arcs &arcs_out = abs_state->get_arcs_out();
        for (Arcs::iterator it = arcs_out.begin(); it != arcs_out.end(); ++it) {
            const GlobalOperator *op = it->first;
            AbstractState *next_abs = it->second;
            assert(!use_astar || (abs_state->get_next_solution_op() &&
                                  abs_state->get_next_solution_state()));
            if (use_astar && (op != abs_state->get_next_solution_op() ||
                              next_abs != abs_state->get_next_solution_state()))
                continue;
            if (next_abs->get_h() + op->get_cost() != abs_state->get_h())
                // GlobalOperator is not part of an optimal path.
                continue;
            if (op->is_applicable(conc_state)) {
                if (DEBUG)
                    cout << "      Move to: " << next_abs->str()
                         << " with " << op->get_name() << endl;
                GlobalState next_conc = registry->get_successor_state(conc_state, *op);
                if (next_abs->is_abstraction_of(next_conc)) {
                    if (seen.count(next_conc.get_id()) == 0) {
                        unseen.push(make_pair(next_abs, next_conc));
                        seen.insert(next_conc.get_id());
                    }
                } else if (splits.empty()) {
                    // Only find deviation reasons if we haven't found any splits already.
                    if (DEBUG)
                        cout << "      Paths deviate." << endl;
                    ++deviations;
                    AbstractState desired_abs_state(task_proxy);
                    next_abs->regress(*op, &desired_abs_state);
                    abs_state->get_possible_splits(desired_abs_state, conc_state,
                                                   &splits);
                }
            } else if (splits.empty()) {
                // Only find unmet preconditions if we haven't found any splits already.
                if (DEBUG)
                    cout << "      GlobalOperator not applicable: " << op->get_name() << endl;
                ++unmet_preconditions;
                get_unmet_preconditions(*op, conc_state, &splits);
            }
        }
    }
    int broken_solutions = 0;
    for (StatesToSplits::iterator it = states_to_splits.begin(); it != states_to_splits.end(); ++it) {
        if (!may_keep_refining())
            break;
        AbstractState *state = it->first;
        Splits &splits = it->second;
        random_shuffle(splits.begin(), splits.end());
        if (!splits.empty()) {
            break_solution(state, splits);
            ++broken_solutions;
        }
    }
    if (DEBUG)
        cout << "Broke " << broken_solutions << " solutions" << endl;
    assert(broken_solutions > 0 || !may_keep_refining());
    assert(!use_astar || broken_solutions == 1 || !may_keep_refining());
    return false;
}

int Abstraction::pick_split_index(AbstractState &state, const Splits &splits) const {
    // TODO: Return reference to split instead if index and rename mathod.
    assert(!splits.empty());
    // Shortcut for condition lists with only one element.
    if (splits.size() == 1) {
        return 0;
    }
    if (DEBUG) {
        cout << "Split: " << state.str() << endl;
        for (size_t i = 0; i < splits.size(); ++i) {
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
    int random_cond = rng(splits.size());
    if (pick == RANDOM) {
        cond = random_cond;
    } else if (pick == MIN_CONSTRAINED || pick == MAX_CONSTRAINED) {
        int max_rest = -1;
        int min_rest = INF;
        for (size_t i = 0; i < splits.size(); ++i) {
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
        int i = 0;
        for (auto split : splits) {
            int var_id = split.first;
            double all_values = task_proxy.get_variables()[var_id].get_domain_size();
            double rest = state.count(var_id);
            assert(all_values >= 2);
            assert(rest >= 2);
            assert(rest <= all_values);
            double refinement = -(rest / all_values);
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
            ++i;
        }
    } else if (pick == MIN_HADD || pick == MAX_HADD) {
        int min_hadd = INF;
        int max_hadd = -2;
        for (size_t i = 0; i < splits.size(); ++i) {
            int var = splits[i].first;
            const vector<int> &values = splits[i].second;
            for (size_t j = 0; j < values.size(); ++j) {
                int value = values[j];
                int hadd_value = task->get_hadd_value(var, value);
                if (hadd_value == -1 && pick == MIN_HADD) {
                    // Fact is unreachable --> Choose it last.
                    hadd_value = INF - 1;
                }
                if (pick == MIN_HADD && hadd_value < min_hadd) {
                    cond = i;
                    min_hadd = hadd_value;
                } else if (pick == MAX_HADD && hadd_value > max_hadd) {
                    cond = i;
                    max_hadd = hadd_value;
                }
            }
        }
    } else {
        cout << "Invalid pick strategy: " << pick << endl;
        exit_with(EXIT_INPUT_ERROR);
    }
    assert(in_bounds(cond, splits));
    return cond;
}

void Abstraction::extract_solution(AbstractState *goal) const {
    assert(get_min_goal_distance() != INF);
    AbstractState *current = goal;
    while (current != init) {
        const GlobalOperator *prev_op = current->get_prev_solution_op();
        AbstractState *prev_state = current->get_prev_solution_state();
        prev_state->set_next_solution_op(prev_op);
        prev_state->set_next_solution_state(current);
        prev_state->set_h(current->get_h() + prev_op->get_cost());
        assert(prev_state != current);
        current = prev_state;
    }
    assert(init->get_h() == get_min_goal_distance());
}

void Abstraction::find_solution() const {
    reset_distances_and_solution();
    open->clear();
    init->set_distance(0);
    open->push(init->get_h(), init);
    astar_search(true, true);
}

void Abstraction::update_h_values() const {
    reset_distances_and_solution();
    open->clear();
    for (AbstractStates::const_iterator it = goals.begin(); it != goals.end(); ++it) {
        (*it)->set_distance(0);
        open->push(0, *it);
    }
    astar_search(false, false);
    for (AbstractStates::const_iterator it = states.begin(); it != states.end(); ++it) {
        AbstractState *state = *it;
        state->set_h(state->get_distance());
    }
}

double Abstraction::get_avg_h() const {
    assert(!memory_released);
    // This function is called after the abstraction has been built, so all
    // h-values are up-to-date.
    double avg_h = 0.;
    for (AbstractStates::const_iterator it = states.begin(); it != states.end(); ++it) {
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

int Abstraction::get_init_h() const {
    assert(!memory_released);
    return init->get_h();
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
        for (Arcs::iterator it = next.begin(); it != next.end(); ++it) {
            const GlobalOperator *op = it->first;
            AbstractState *next_state = it->second;
            dotfile << current_state->str() << " -> " << next_state->str()
                    << " [label=\"" << op->get_name() << "\"];" << endl;
        }
        if (draw_loops) {
            Loops &loops = current_state->get_loops();
            for (Loops::iterator it = loops.begin(); it != loops.end(); ++it) {
                const GlobalOperator *op = *it;
                dotfile << current_state->str() << " -> " << current_state->str()
                        << " [label=\"" << op->get_name() << "\"];" << endl;
            }
        }
        if (current_state == init) {
            dotfile << current_state->str() << " [color=green];" << endl;
        } else if (is_goal(current_state)) {
            dotfile << current_state->str() << " [color=red];" << endl;
        }
    }
    dotfile << "}" << endl;
    dotfile.close();
}

int Abstraction::get_op_index(const GlobalOperator *op) const {
    return (*op_to_index.find(op)).second;
}

void Abstraction::get_needed_costs(vector<int> *needed_costs) {
    assert(needed_costs->empty());
    needed_costs->resize(task_proxy.get_operators().size(), -MAX_COST_VALUE);
    // Traverse abstraction and remember the minimum cost we need to keep for
    // each operator in order not to decrease any heuristic values.
    open->clear();
    reset_distances_and_solution();
    init->set_distance(0);
    open->push(0, init);
    astar_search(true, false, needed_costs);
}

bool Abstraction::may_keep_refining() const {
    return memory_padding_is_reserved() &&
           (get_num_states() < max_states) &&
           (max_time == INF || timer() < max_time);
}

void Abstraction::release_memory() {
    if (DEBUG)
        cout << "Release memory" << endl;
    assert(!memory_released);
    delete open;
    open = 0;
    release_memory_padding();
    AbstractStates::iterator it;
    for (it = states.begin(); it != states.end(); ++it) {
        AbstractState *state = *it;
        delete state;
    }
    AbstractStates().swap(states);
    delete registry;
    memory_released = true;
}

void Abstraction::print_statistics() {
    int nexts = 0, prevs = 0, total_loops = 0;
    int dead_ends = 0;
    int arc_size = 0;
    for (AbstractStates::iterator it = states.begin(); it != states.end(); ++it) {
        AbstractState *state = *it;
        if (state->get_h() == INF)
            ++dead_ends;
        Arcs &next = state->get_arcs_out();
        Arcs &prev = state->get_arcs_in();
        Loops &loops = state->get_loops();
        nexts += next.size();
        prevs += prev.size();
        total_loops += loops.size();
        arc_size += sizeof(next) + sizeof(Arc) * next.capacity() +
                    sizeof(prev) + sizeof(Arc) * prev.capacity() +
                    sizeof(loops) + sizeof(GlobalOperator *) * loops.capacity();
    }
    assert(nexts == prevs);

    cout << "Time: " << timer << endl;
    cout << "Peak memory: " << get_peak_memory_in_kb() << " KB" << endl;
    cout << "States: " << num_states << endl;
    cout << "Dead-ends: " << dead_ends << endl;
    cout << "Init-h: " << init->get_h() << endl;
    cout << "Avg-h:  " << get_avg_h() << endl;

    cout << "Transitions: " << nexts << endl;
    cout << "Self-loops: " << total_loops << endl;
    cout << "Arc size: " << arc_size / 1024 << " KB" << endl;

    cout << "Deviations: " << deviations << endl;
    cout << "Unmet preconditions: " << unmet_preconditions << endl;
    cout << "Unmet goals: " << unmet_goals << endl;
}

void Abstraction::print_histograms() const {
    map<int, int> h_to_abs_states;
    map<int, double> h_to_rel_conc_states;
    for (AbstractStates::const_iterator it = states.begin(); it != states.end(); ++it) {
        AbstractState *state = *it;
        const int h = state->get_h();
        assert(h >= 0);
        h_to_abs_states[h] += 1;
        h_to_rel_conc_states[h] += state->get_rel_conc_states();
    }
    cout << "Number of abstract states" << endl;
    for (map<int, int>::iterator it = h_to_abs_states.begin(); it != h_to_abs_states.end(); ++it) {
        cout << "h=" << left << setw(3) << it->first << ": " << it->second << endl;
    }
    cout << "Percentage of concrete states" << endl;
    for (map<int, double>::iterator it = h_to_rel_conc_states.begin(); it != h_to_rel_conc_states.end(); ++it) {
        cout << "h=" << left << setw(3) << it->first << ": " << fixed << 100 * it->second << endl;
    }
}
}

#include "abstraction.h"

#include "abstract_state.h"
#include "task.h"
#include "utils.h"
#include "../additive_heuristic.h"
#include "../globals.h"
#include "../operator.h"
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
#include <tr1/unordered_map>
#include <utility>
#include <vector>

using namespace std;
using namespace std::tr1;

namespace cegar_heuristic {
typedef unordered_map<AbstractState *, Splits> StatesToSplits;

static char *cegar_memory_padding = 0;

// Memory paddings of <= 50 MB often result in the run being killed, just
// because it is very close to its memory limit. Setting this to 75 MB
// avoids this issue. The reason for this behaviour seems to be that the
// search needs adjacent memory.
static const int MEMORY_PADDING_MB = 75;

// Save previous out-of-memory handler.
static void (*global_out_of_memory_handler)(void) = 0;

void no_memory_continue() {
    assert(cegar_memory_padding);
    delete[] cegar_memory_padding;
    cegar_memory_padding = 0;
    cout << "Failed to allocate memory for CEGAR abstraction. "
         << "Released memory padding and will stop refinement now." << endl;
    assert(global_out_of_memory_handler);
    set_new_handler(global_out_of_memory_handler);
}

Abstraction::Abstraction(const Task *t)
    : task(t),
      single(new AbstractState()),
      init(single),
      goal(single),
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
      write_dot_files(false),
      memory_released(false) {
    assert(!task->get_goal().empty());

    assert(!cegar_memory_padding);
    if (DEBUG)
        cout << "Reserving " << MEMORY_PADDING_MB << " MB of memory padding." << endl;
    cegar_memory_padding = new char[MEMORY_PADDING_MB * 1024 * 1024];
    // TODO: Move into abstraction class.
    global_out_of_memory_handler = set_new_handler(no_memory_continue);

    split_tree.set_root(single);
    for (int i = 0; i < task->get_operators().size(); ++i) {
        single->add_loop(&task->get_operators()[i]);
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

void Abstraction::build() {
    if (write_dot_files) {
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
        valid_conc_solution = check_and_break_solution(g_initial_state(), init);
        if (valid_conc_solution)
            break;
    }
    // Remember number of states before we release the memory.
    num_states = get_num_states();
    assert(num_states == states.size());
    cout << "Done building abstraction [t=" << g_timer << "]" << endl;
    cout << "Peak memory: " << get_peak_memory_in_kb() << " KB" << endl;
    cout << "Solution found while refining: " << valid_conc_solution << endl;
    cout << "Single abstraction states: " << num_states << endl;

    // Even if we found a valid concrete solution, we might have refined in the
    // last iteration, so we must update the h-values.
    update_h_values();
    cout << "Single abstraction init-h: " << init->get_h() << endl;
    cout << "Single abstraction avg-h:  " << get_avg_h() << endl;
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

    // TODO: Since the search is always started from the abstract initial state
    // we can assume v2 is never "init" and v1 is never "goal".
    if (state == init) {
        if (v1->is_abstraction_of(g_initial_state())) {
            assert(!v2->is_abstraction_of(g_initial_state()));
            init = v1;
        } else {
            assert(v2->is_abstraction_of(g_initial_state()));
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
             << max_states << endl;
    if (write_dot_files)
        write_dot_file(num_states);
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
        for (Arcs::iterator it = successors.begin(); it != successors.end(); ++it) {
            const Operator *op = it->first;
            AbstractState *successor = it->second;

            // Special code for additive abstractions.
            if (needed_costs) {
                assert(forward);
                assert(!use_h);
                // We are currently collecting the needed operator costs.
                assert(needed_costs->size() == task->get_operators().size());
                // cost'(op) = h(a1) - h(a2)
                const int needed = state->get_h() - successor->get_h();
                if (needed > 0) {
                    // needed_costs is negative if we reach a2 with op and
                    // h(a2) > h(a1). This includes the case when we reach a
                    // dead-end node. If h(a1)==h(a2) we don't have to update
                    // anything since we initialize the list with zeros. This
                    // handles moving from one dead-end node to another.
                    const int op_index = get_op_index(op);
                    (*needed_costs)[op_index] = max((*needed_costs)[op_index], needed);
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
    assert(abs_state->is_abstraction_of(conc_state));

    if (DEBUG)
        cout << "Check solution." << endl << "Start at       " << abs_state->str()
             << " (is init: " << (abs_state == init) << ")" << endl;

    StatesToSplits states_to_splits;
    queue<pair<AbstractState *, State> > unseen;
    unordered_set<StateID, hash_state_id> seen;

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
        for (Arcs::iterator it = arcs_out.begin(); it != arcs_out.end(); ++it) {
            const Operator *op = it->first;
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
                State next_conc = task->get_state_registry()->get_successor_state(conc_state, *op);
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
                    AbstractState desired_abs_state;
                    next_abs->regress(*op, &desired_abs_state);
                    abs_state->get_possible_splits(desired_abs_state, conc_state,
                                                   &splits);
                    restrict_splits(conc_state, splits);
                }
            } else if (splits.empty()) {
                // Only find unmet preconditions if we haven't found any splits already.
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

void Abstraction::restrict_splits(State &conc_state, Splits &splits) const {
    if (splits.size() == 1)
        return;
    if (pick == MIN_HADD_DYN || pick == MAX_HADD_DYN) {
        Options opts;
        opts.set<int>("cost_type", 0);
        opts.set<int>("memory_padding", 75);
        AdditiveHeuristic hadd(opts);
        hadd.evaluate(conc_state);
        int min_hadd = INF;
        int max_hadd = -1;
        vector<int> incumbents;
        for (int i = 0; i < splits.size(); ++i) {
            int var = splits[i].first;
            const vector<int> &values = splits[i].second;
            for (int j = 0; j < values.size(); ++j) {
                int value = values[j];
                int hadd_value = hadd.get_cost(var, value);
                assert(hadd_value != -1);
                if (pick == MIN_HADD_DYN) {
                    if (hadd_value < min_hadd) {
                        incumbents.clear();
                        min_hadd = hadd_value;
                    }
                    if (hadd_value <= min_hadd)
                        incumbents.push_back(i);
                } else {
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
    if (pick == RANDOM) {
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
            double all_values = g_variable_domain[splits[i].first];
            double rest = state.count(splits[i].first);
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
        }
    } else if (pick == MIN_HADD || pick == MAX_HADD) {
        int min_hadd = INF;
        int max_hadd = -2;
        for (int i = 0; i < splits.size(); ++i) {
            int var = splits[i].first;
            const vector<int> &values = splits[i].second;
            for (int j = 0; j < values.size(); ++j) {
                int value = values[j];
                int hadd_value = task->get_hadd_value(var, value);
                if (hadd_value == -1) {
                    // Fact is unreachable --> Choose it last.
                    if (pick == MIN_HADD)
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
        const Operator *prev_op = current->get_prev_solution_op();
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
    for (AbstractStates::const_iterator it = states.begin(); it != states.end(); ++it) {
        AbstractState *state = *it;
        state->set_h(state->get_distance());
    }
}

double Abstraction::get_avg_h() const {
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
            const Operator *op = it->first;
            AbstractState *next_state = it->second;
            dotfile << current_state->str() << " -> " << next_state->str()
                    << " [label=\"" << op->get_name() << "\"];" << endl;
        }
        if (draw_loops) {
            Loops &loops = current_state->get_loops();
            for (Loops::iterator it = loops.begin(); it != loops.end(); ++it) {
                const Operator *op = *it;
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
    int op_index = op - &*task->get_operators().begin();
    assert(op_index >= 0 && op_index < task->get_operators().size());
    return op_index;
}

void Abstraction::get_needed_costs(vector<int> *needed_costs) {
    assert(needed_costs->empty());
    needed_costs->resize(task->get_operators().size(), 0);
    // Traverse abstraction and remember the minimum cost we need to keep for
    // each operator in order not to decrease any heuristic values.
    open->clear();
    reset_distances_and_solution();
    init->set_distance(0);
    open->push(0, init);
    astar_search(true, false, needed_costs);
}

bool Abstraction::may_keep_refining() const {
    return cegar_memory_padding &&
           (get_num_states() < max_states) &&
           (max_time == INF || timer() < max_time);
}

void Abstraction::release_memory() {
    if (DEBUG)
        cout << "Release memory" << endl;
    assert(!memory_released);
    delete open;
    open = 0;
    if (cegar_memory_padding) {
        delete[] cegar_memory_padding;
        cegar_memory_padding = 0;
    }
    assert(global_out_of_memory_handler);
    set_new_handler(global_out_of_memory_handler);
    AbstractStates::iterator it;
    for (it = states.begin(); it != states.end(); ++it) {
        AbstractState *state = *it;
        delete state;
    }
    AbstractStates().swap(states);
    memory_released = true;
}

void Abstraction::print_statistics() {
    int nexts = 0, prevs = 0, total_loops = 0;
    int unreachable_states = 0;
    int arc_size = 0;
    for (AbstractStates::iterator it = states.begin(); it != states.end(); ++it) {
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
}
}

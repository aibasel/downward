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
using StatesToFlaws = unordered_map<AbstractState *, Flaws>;

Abstraction::Abstraction(const Options &opts)
    : task_proxy(*opts.get<TaskProxy *>("task_proxy")),
      do_separate_unreachable_facts(opts.get<bool>("separate_unreachable_facts")),
      flaw_selector(task_proxy, PickFlaw(opts.get<int>("pick"))),
      max_states(opts.get<int>("max_states")),
      use_astar(opts.get<bool>("use_astar")),
      use_general_costs(opts.get<bool>("use_general_costs")),
      timer(opts.get<double>("max_time")),
      concrete_initial_state(task_proxy.get_initial_state()),
      init(nullptr),
      deviations(0),
      unmet_preconditions(0),
      unmet_goals(0) {
    reserve_memory_padding();

    build();

    // Even if we found a concrete solution, we might have refined in the
    // last iteration, so we must update the h values.
    update_h_values();

    print_statistics();
}

Abstraction::~Abstraction() {
    release_memory_padding();
    for (AbstractState *state : states)
        delete state;
}

int Abstraction::get_min_goal_distance() const {
    int min_distance = INF;
    for (AbstractState *goal : goals) {
        min_distance = min(min_distance, goal->get_distance());
    }
    return min_distance;
}

bool Abstraction::is_goal(AbstractState *state) const {
    return goals.count(state) == 1;
}

void Abstraction::separate_unreachable_facts() {
    assert(states.size() == 1);
    assert(task_proxy.get_goals().size() == 1);
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

void Abstraction::create_initial_abstraction() {
    init = new AbstractState(Domains(task_proxy), split_tree.get_root());
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
    create_initial_abstraction();
    bool found_conc_solution = false;
    while (may_keep_refining()) {
        if (use_astar) {
            find_solution();
        } else {
            update_h_values();
        }
        if ((use_astar && get_min_goal_distance() == INF) ||
            (!use_astar && init->get_h() == INF)) {
            cout << "Abstract problem is unsolvable!" << endl;
            found_conc_solution = false;
            break;
        }
        found_conc_solution = check_and_break_solution(concrete_initial_state, init);
        if (found_conc_solution)
            break;
    }
    cout << "Solution found while refining: " << found_conc_solution << endl;
}

void Abstraction::break_solution(AbstractState *state, const Flaws &flaws) {
    const Flaw &flaw = flaw_selector.pick_flaw(*state, flaws);
    refine(state, flaw.first, flaw.second);
}

void Abstraction::refine(AbstractState *state, int var, const vector<int> &wanted) {
    assert(may_keep_refining());
    if (DEBUG)
        cout << "Refine " << state->str() << " for "
             << var << "=" << wanted << endl;
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

    delete state;
}

void Abstraction::reset_distances_and_solution() const {
    for (AbstractState *state : states) {
        state->set_distance(INF);
    }
    solution_backward.clear();
    solution_forward.clear();
}

void Abstraction::astar_search(bool forward, bool use_h, vector<int> *needed_costs) const {
    bool debug = true && DEBUG;
    while (!open_queue.empty()) {
        pair<int, AbstractState *> top_pair = open_queue.pop();
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
            return;
        }
        // All operators that induce self-loops need at least 0 costs.
        if (needed_costs) {
            assert(forward);
            assert(!use_h);
            assert(needed_costs->size() == task_proxy.get_operators().size());
            for (OperatorProxy op : state->get_loops()) {
                (*needed_costs)[op.get_id()] = max((*needed_costs)[op.get_id()], 0);
            }
        }
        Arcs &successors = (forward) ? state->get_arcs_out() : state->get_arcs_in();
        for (auto &arc : successors) {
            OperatorProxy op = arc.first;
            AbstractState *successor = arc.second;

            // Collect needed operator costs for additive abstractions.
            if (needed_costs) {
                assert(forward);
                assert(!use_h);
                assert(needed_costs->size() == task_proxy.get_operators().size());
                int needed = state->get_h() - successor->get_h();
                if (!use_general_costs)
                    needed = max(0, needed);
                (*needed_costs)[op.get_id()] = max((*needed_costs)[op.get_id()], needed);
            }

            assert(op.get_cost() >= 0);
            int succ_g = g + op.get_cost();
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
                open_queue.push(f, successor);
                auto it = solution_backward.find(successor);
                if (it != solution_backward.end())
                    solution_backward.erase(it);
                solution_backward.insert(make_pair(successor, Arc(op, state)));
            }
        }
    }
}

bool Abstraction::check_and_break_solution(State conc_state, AbstractState *abs_state) {
    assert(abs_state->is_abstraction_of(conc_state));

    if (DEBUG)
        cout << "Check solution." << endl << "Start at       " << abs_state->str()
             << " (is init: " << (abs_state == init) << ")" << endl;

    StatesToFlaws states_to_flaws;
    queue<pair<AbstractState *, State> > unseen;
    unordered_set<size_t> seen;

    unseen.push(make_pair(abs_state, conc_state));

    // Only search flaws until we hit the memory limit.
    while (!unseen.empty() && memory_padding_is_reserved()) {
        abs_state = unseen.front().first;
        conc_state = move(unseen.front().second);
        unseen.pop();
        Flaws &flaws = states_to_flaws[abs_state];
        if (DEBUG)
            cout << "Current state: " << abs_state->str() << endl;
        // Start check from each state only once.
        if (!states_to_flaws[abs_state].empty())
            continue;
        if (is_goal(abs_state)) {
            if (is_goal_state(task_proxy, conc_state)) {
                // We found a valid concrete solution.
                return true;
            } else {
                // Get unmet goals and refine the last state.
                if (DEBUG)
                    cout << "      Goal test failed." << endl;
                unmet_goals++;
                get_unmet_goals(task_proxy.get_goals(), conc_state, &states_to_flaws[abs_state]);
                continue;
            }
        }
        Arcs &arcs_out = abs_state->get_arcs_out();
        for (auto &arc : arcs_out) {
            OperatorProxy op = arc.first;
            AbstractState *next_abs = arc.second;
            assert(!use_astar || solution_forward.count(abs_state) == 1);
            if (use_astar && solution_forward.at(abs_state) != arc)
                continue;
            if (next_abs->get_h() + op.get_cost() != abs_state->get_h())
                // Operator is not part of an optimal path.
                continue;
            if (is_applicable(op, conc_state)) {
                if (DEBUG)
                    cout << "      Move to: " << next_abs->str()
                         << " with " << op.get_name() << endl;
                State next_conc = move(conc_state.apply(op));
                if (next_abs->is_abstraction_of(next_conc)) {
                    if (seen.count(next_conc.hash()) == 0) {
                        unseen.push(make_pair(next_abs, next_conc));
                        seen.insert(next_conc.hash());
                    }
                } else if (flaws.empty()) {
                    // Only find deviation reasons if we haven't found any flaws already.
                    if (DEBUG)
                        cout << "      Paths deviate." << endl;
                    ++deviations;
                    AbstractState desired_abs_state(next_abs->regress(op), nullptr);
                    abs_state->get_possible_flaws(
                        desired_abs_state, conc_state, &flaws);
                }
            } else if (flaws.empty()) {
                // Only find unmet preconditions if we haven't found any flaws already.
                if (DEBUG)
                    cout << "      Operator not applicable: " << op.get_name() << endl;
                ++unmet_preconditions;
                get_unmet_preconditions(op, conc_state, &flaws);
            }
        }
    }
    int broken_solutions = 0;
    for (auto &state_and_flaws : states_to_flaws) {
        if (!may_keep_refining())
            break;
        AbstractState *state = state_and_flaws.first;
        Flaws &flaws = state_and_flaws.second;
        if (!flaws.empty()) {
            break_solution(state, flaws);
            ++broken_solutions;
        }
    }
    if (DEBUG)
        cout << "Broke " << broken_solutions << " solutions" << endl;
    assert(broken_solutions > 0 || !may_keep_refining());
    assert(!use_astar || broken_solutions == 1 || !may_keep_refining());
    return false;
}

void Abstraction::extract_solution(AbstractState *goal) const {
    assert(get_min_goal_distance() != INF);
    AbstractState *current = goal;
    while (current != init) {
        Arc &prev_arc = solution_backward.at(current);
        OperatorProxy prev_op = prev_arc.first;
        AbstractState *prev_state = prev_arc.second;
        assert(solution_forward.count(prev_state) == 0);
        solution_forward.insert(make_pair(prev_state, Arc(prev_op, current)));
        prev_state->set_h(current->get_h() + prev_op.get_cost());
        assert(prev_state != current);
        current = prev_state;
    }
    assert(init->get_h() == get_min_goal_distance());
}

void Abstraction::find_solution() const {
    reset_distances_and_solution();
    open_queue.clear();
    init->set_distance(0);
    open_queue.push(init->get_h(), init);
    astar_search(true, true);
}

void Abstraction::update_h_values() const {
    reset_distances_and_solution();
    open_queue.clear();
    for (AbstractState *goal : goals) {
        goal->set_distance(0);
        open_queue.push(0, goal);
    }
    astar_search(false, false);
    for (AbstractState *state : states) {
        state->set_h(state->get_distance());
    }
}

int Abstraction::get_init_h() const {
    return init->get_h();
}

vector<int> Abstraction::get_needed_costs() {
    vector<int> needed_costs(task_proxy.get_operators().size(), -MAX_COST_VALUE);
    // Traverse abstraction and remember the minimum cost we need to keep for
    // each operator in order not to decrease any heuristic values.
    open_queue.clear();
    reset_distances_and_solution();
    init->set_distance(0);
    open_queue.push(0, init);
    astar_search(true, false, &needed_costs);
    return needed_costs;
}

void Abstraction::print_statistics() {
    int nexts = 0, prevs = 0, total_loops = 0;
    int dead_ends = 0;
    int arc_size = 0;
    for (AbstractState *state : states) {
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
                    sizeof(loops) + sizeof(OperatorProxy) * loops.capacity();
    }
    assert(nexts == prevs);

    cout << "Time: " << timer << endl;
    cout << "Peak memory: " << get_peak_memory_in_kb() << " KB" << endl;
    cout << "States: " << get_num_states() << endl;
    cout << "Dead-ends: " << dead_ends << endl;
    cout << "Init-h: " << init->get_h() << endl;

    cout << "Transitions: " << nexts << endl;
    cout << "Self-loops: " << total_loops << endl;
    cout << "Arc size: " << arc_size / 1024 << " KB" << endl;

    cout << "Deviations: " << deviations << endl;
    cout << "Unmet preconditions: " << unmet_preconditions << endl;
    cout << "Unmet goals: " << unmet_goals << endl;
}
}

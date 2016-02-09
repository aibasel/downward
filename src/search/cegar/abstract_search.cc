#include "abstract_search.h"

#include "utils.h"

#include <cassert>

using namespace std;

namespace cegar {
// See additive_heuristic.h.
static const int MAX_COST_VALUE = 100000000;

AbstractSearch::AbstractSearch(
    vector<int> &&operator_costs, bool use_general_costs)
    : operator_costs(move(operator_costs)),
      use_general_costs(use_general_costs) {
}

void AbstractSearch::reset() {
    g_values.clear();
    open_queue.clear();
    prev_arc.clear();
    solution.clear();
}

bool AbstractSearch::find_solution(AbstractState *init, AbstractStates &goals) {
    reset();
    g_values[init] = 0;
    open_queue.push(init->get_h_value(), init);
    AbstractState *goal = astar_search(true, true, &goals);
    bool has_found_solution = static_cast<bool>(goal);
    if (has_found_solution) {
        extract_solution(init, goal);
    }
    return has_found_solution;
}

void AbstractSearch::backwards_dijkstra(const AbstractStates goals) {
    reset();
    for (AbstractState *goal : goals) {
        g_values[goal] = 0;
        open_queue.push(0, goal);
    }
    astar_search(false, false);
}

vector<int> AbstractSearch::get_needed_costs(AbstractState *init, int num_ops) {
    reset();
    vector<int> needed_costs(num_ops, -MAX_COST_VALUE);
    g_values[init] = 0;
    open_queue.push(0, init);
    astar_search(true, false, nullptr, &needed_costs);
    return needed_costs;
}

int AbstractSearch::get_g_value(AbstractState *state) const {
    auto it = g_values.find(state);
    if (it == g_values.end())
        return INF;
    return it->second;
}

AbstractState *AbstractSearch::astar_search(
    bool forward, bool use_h, AbstractStates *goals, vector<int> *needed_costs) {
    assert((forward && use_h && goals && !needed_costs) ||
           (!forward && !use_h && !goals && !needed_costs) ||
           (forward && !use_h && !goals && needed_costs));
    while (!open_queue.empty()) {
        pair<int, AbstractState *> top_pair = open_queue.pop();
        int old_f = top_pair.first;
        AbstractState *state = top_pair.second;

        const int g = get_g_value(state);
        assert(0 <= g && g < INF);
        int new_f = g;
        if (use_h)
            new_f += state->get_h_value();
        assert(new_f <= old_f);
        if (new_f < old_f)
            continue;
        if (goals && goals->count(state) == 1) {
            return state;
        }
        if (needed_costs) {
            /* To prevent negative cost cycles, all operators inducing
               self-loops must have non-negative costs. */
            for (int op_id : state->get_loops())
                (*needed_costs)[op_id] = max((*needed_costs)[op_id], 0);
        }
        const Arcs &successors = (forward) ? state->get_outgoing_arcs() :
                                 state->get_incoming_arcs();
        for (auto &arc : successors) {
            int op_id = arc.first;
            AbstractState *successor = arc.second;

            if (needed_costs) {
                int needed = state->get_h_value() - successor->get_h_value();
                if (!use_general_costs)
                    needed = max(0, needed);
                (*needed_costs)[op_id] = max((*needed_costs)[op_id], needed);
            }

            assert(utils::in_bounds(op_id, operator_costs));
            const int op_cost = operator_costs[op_id];
            assert(op_cost >= 0);
            int succ_g = g + op_cost;
            assert(succ_g >= 0);

            if (succ_g < get_g_value(successor)) {
                g_values[successor] = succ_g;
                int f = succ_g;
                if (use_h) {
                    int h = successor->get_h_value();
                    if (h == INF)
                        continue;
                    f += h;
                }
                assert(f >= 0);
                open_queue.push(f, successor);
                // Work around the fact that OperatorProxy has no default constructor.
                auto it = prev_arc.find(successor);
                if (it != prev_arc.end())
                    prev_arc.erase(it);
                prev_arc.insert(make_pair(successor, Arc(op_id, state)));
            }
        }
    }
    return nullptr;
}

void AbstractSearch::extract_solution(AbstractState *init, AbstractState *goal) {
    AbstractState *current = goal;
    while (current != init) {
        Arc &prev = prev_arc.at(current);
        int prev_op_id = prev.first;
        AbstractState *prev_state = prev.second;
        solution.push_front(Arc(prev_op_id, current));
        assert(utils::in_bounds(prev_op_id, operator_costs));
        const int prev_op_cost = operator_costs[prev_op_id];
        prev_state->set_h_value(current->get_h_value() + prev_op_cost);
        assert(prev_state != current);
        current = prev_state;
    }
}
}

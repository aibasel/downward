#include "abstract_search.h"

#include "abstract_state.h"
#include "utils.h"

#include "../option_parser.h"

#include <cassert>

using namespace std;

namespace cegar {
AbstractSearch::AbstractSearch(const Options &opts)
    : use_general_costs(opts.get<bool>("use_general_costs")) {
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
    open_queue.push(init->get_h(), init);
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
    // Traverse abstraction and remember the minimum cost we need to keep for
    // each operator in order not to decrease any heuristic values.
    g_values[init] = 0;
    open_queue.push(0, init);
    astar_search(true, false, nullptr, &needed_costs);
    return needed_costs;
}

int AbstractSearch::get_g(AbstractState *state) const {
    if (g_values.count(state) == 0)
        return INF;
    return g_values.at(state);
}

AbstractState *AbstractSearch::astar_search(
    bool forward, bool use_h, AbstractStates *goals, vector<int> *needed_costs) {
    if (forward && use_h) {
        assert(goals);
    }
    if (needed_costs) {
        assert(forward && !use_h);
    }
    while (!open_queue.empty()) {
        pair<int, AbstractState *> top_pair = open_queue.pop();
        int old_f = top_pair.first;
        AbstractState *state = top_pair.second;

        const int g = get_g(state);
        assert(0 <= g && g < INF);
        int new_f = g;
        if (use_h)
            new_f += state->get_h();
        assert(new_f <= old_f);
        if (new_f < old_f)
            continue;
        if (forward && use_h && goals && goals->count(state) == 1) {
            return state;
        }
        // All operators that induce self-loops need at least 0 costs.
        if (needed_costs) {
            for (OperatorProxy op : state->get_loops())
                (*needed_costs)[op.get_id()] = max((*needed_costs)[op.get_id()], 0);
        }
        const Arcs &successors = (forward) ? state->get_outgoing_arcs() : state->get_incoming_arcs();
        for (auto &arc : successors) {
            OperatorProxy op = arc.first;
            AbstractState *successor = arc.second;

            // Collect needed operator costs for additive AbstractSearchs.
            if (needed_costs) {
                int needed = state->get_h() - successor->get_h();
                if (!use_general_costs)
                    needed = max(0, needed);
                (*needed_costs)[op.get_id()] = max((*needed_costs)[op.get_id()], needed);
            }

            assert(op.get_cost() >= 0);
            int succ_g = g + op.get_cost();
            assert(succ_g >= 0);

            if (succ_g < get_g(successor)) {
                g_values[successor] = succ_g;
                int f = succ_g;
                if (use_h) {
                    int h = successor->get_h();
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
                prev_arc.insert(make_pair(successor, Arc(op, state)));
            }
        }
    }
    return nullptr;
}

void AbstractSearch::extract_solution(AbstractState *init, AbstractState *goal) {
    AbstractState *current = goal;
    while (current != init) {
        Arc &prev = prev_arc.at(current);
        OperatorProxy prev_op = prev.first;
        AbstractState *prev_state = prev.second;
        solution.push_front(Arc(prev_op, current));
        prev_state->set_h(current->get_h() + prev_op.get_cost());
        assert(prev_state != current);
        current = prev_state;
    }
}
}

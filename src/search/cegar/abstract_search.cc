#include "abstract_search.h"

#include "abstract_state.h"
#include "utils.h"

#include <cassert>

using namespace std;

namespace cegar {
AbstractSearch::AbstractSearch(
    vector<int> &&operator_costs,
    AbstractStates &states)
    : operator_costs(move(operator_costs)),
      states(states) {
}

void AbstractSearch::reset() {
    open_queue.clear();
    for (AbstractState *state : states) {
        state->get_search_info().reset();
    }
    solution.clear();
}

bool AbstractSearch::find_solution(AbstractState *init, AbstractStates &goals) {
    reset();
    init->get_search_info().decrease_g_value_to(0);
    open_queue.push(init->get_h_value(), init);
    AbstractState *goal = astar_search(true, true, &goals);
    bool has_found_solution = static_cast<bool>(goal);
    if (has_found_solution) {
        extract_solution(init, goal);
    }
    return has_found_solution;
}

void AbstractSearch::forward_dijkstra(AbstractState *init) {
    reset();
    init->get_search_info().decrease_g_value_to(0);
    open_queue.push(0, init);
    astar_search(true, false);
}

void AbstractSearch::backwards_dijkstra(const AbstractStates goals) {
    reset();
    for (AbstractState *goal : goals) {
        goal->get_search_info().decrease_g_value_to(0);
        open_queue.push(0, goal);
    }
    astar_search(false, false);
}

AbstractState *AbstractSearch::astar_search(
    bool forward, bool use_h, AbstractStates *goals) {
    assert((forward && use_h && goals) ||
           (!forward && !use_h && !goals) ||
           (forward && !use_h && !goals));
    while (!open_queue.empty()) {
        pair<int, AbstractState *> top_pair = open_queue.pop();
        int old_f = top_pair.first;
        AbstractState *state = top_pair.second;

        const int g = state->get_search_info().get_g_value();
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
        const Transitions &transitions = forward ?
                                         state->get_outgoing_transitions() :
                                         state->get_incoming_transitions();
        for (const Transition &transition : transitions) {
            int op_id = transition.op_id;
            AbstractState *successor = transition.target;

            assert(utils::in_bounds(op_id, operator_costs));
            const int op_cost = operator_costs[op_id];
            assert(op_cost >= 0);
            int succ_g = (op_cost == INF) ? INF : g + op_cost;
            assert(succ_g >= 0);

            if (succ_g < successor->get_search_info().get_g_value()) {
                successor->get_search_info().decrease_g_value_to(succ_g);
                int f = succ_g;
                if (use_h) {
                    int h = successor->get_h_value();
                    if (h == INF)
                        continue;
                    f += h;
                }
                assert(f >= 0);
                open_queue.push(f, successor);
                successor->get_search_info().set_incoming_transition(
                    Transition(op_id, state));
            }
        }
    }
    return nullptr;
}

void AbstractSearch::extract_solution(
    AbstractState *init, AbstractState *goal) {
    AbstractState *current = goal;
    while (current != init) {
        const Transition &prev =
            current->get_search_info().get_incoming_transition();
        solution.emplace_front(prev.op_id, current);
        assert(utils::in_bounds(prev.op_id, operator_costs));
        const int prev_op_cost = operator_costs[prev.op_id];
        assert(prev_op_cost != INF);
        prev.target->set_h_value(current->get_h_value() + prev_op_cost);
        assert(prev.target != current);
        current = prev.target;
    }
}
}

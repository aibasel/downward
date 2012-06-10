#include "./abstraction.h"

#include "./../globals.h"
#include "./../operator.h"
#include "./../state.h"
#include "./../priority_queue.h"

#include <assert.h>

#include <limits>
#include <utility>
#include <iostream>
#include <algorithm>
#include <set>
#include <vector>
using namespace std;

namespace cegar_heuristic {

Abstraction::Abstraction() {
    assert(g_operators.size() > 0);
    init = AbstractState();
    for (int i = 0; i < g_operators.size(); ++i) {
        init.add_arc(g_operators[i], init);
    }
}

bool Abstraction::find_solution() {
    AdaptiveQueue<AbstractState> queue;
    init.set_distance(0);
    init.set_origin(0);
    queue.push(0, init);

    while (!queue.empty()) {
        pair<int, AbstractState> top_pair = queue.pop();
        int distance = top_pair.first;
        AbstractState state = top_pair.second;
        int state_distance = state.get_distance();
        assert(state_distance <= distance);
        if (state_distance < distance) {
            continue;
        }
        if (state.goal_reached()) {
            extract_solution(state);
            return true;
        }
        for (int i = 0; i < state.get_next().size(); i++) {
            const Arc arc = state.get_next()[i];
            Operator op = arc.first;
            AbstractState successor = arc.second;
            int cost = op.get_cost();
            int successor_cost = state_distance + cost;
            if (successor.get_distance() > successor_cost) {
                successor.set_distance(successor_cost);
                Arc origin = Arc(op, state);
                successor.set_origin(&origin);
                queue.push(successor_cost, successor);
            }
        }
    }
    return false;
}

void Abstraction::extract_solution(AbstractState &goal) {
    solution_states.clear();
    solution_ops.clear();

    AbstractState *current = &goal;
    solution_states.push_front(current);
    while (current->get_origin()) {
        Operator op = current->get_origin()->first;
        AbstractState prev = current->get_origin()->second;
        solution_states.push_front(&prev);
        solution_ops.push_front(&op);
        current = &prev;
    }

}

}

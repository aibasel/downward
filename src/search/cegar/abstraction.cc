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
    single = AbstractState();
    for (int i = 0; i < g_operators.size(); ++i) {
        single.add_arc(g_operators[i], single);
    }
    init = &single;
}

void Abstraction::refine(AbstractState *state, int var, int value) {
    assert(g_operators.size() > 0); // We need operators and the g_initial_state
    AbstractState *v1 = new AbstractState();
    AbstractState *v2 = new AbstractState();
    state->refine(var, value, v1, v2);
    if (state == init) {
        if (v1->is_abstraction_of(*g_initial_state)) {
            init = v1;
        }
        else {
            assert(v2->is_abstraction_of(*g_initial_state));
            init = v2;
        }
        cout << "Using new init state: " << init->str() << endl;
    }
}

bool Abstraction::find_solution() {
    AdaptiveQueue<AbstractState*> queue;

    collect_states();
    for (int i = 0; i < abs_states.size(); ++i) {
        queue.push(numeric_limits<int>::max(), abs_states[i]); // TODO: Use pointers
        abs_states[i]->set_distance(numeric_limits<int>::max());
    }

    init->set_distance(0);
    init->set_origin(0);
    queue.push(0, init);

    while (!queue.empty()) {
        pair<int, AbstractState*> top_pair = queue.pop();
        int distance = top_pair.first;
        AbstractState *state = top_pair.second;

        int state_distance = state->get_distance();
        cout << "VISIT: " << state->str() << " " << state_distance << " " << distance << endl;
        assert(state_distance <= distance);
        if (state_distance < distance) {
            continue;
        }
        if (state->goal_reached()) {
            cout << "GOAL REACHED" << endl;
            extract_solution(*state);
            return true;
        }
        for (int i = 0; i < state->get_next().size(); i++) {
            const Arc arc = state->get_next()[i];
            Operator op = arc.first;
            AbstractState successor = arc.second;
            cout << "NEXT: " << successor.str() << endl;
            int cost = op.get_cost();
            int successor_cost = state_distance + cost;
            cout << cost << successor_cost << successor.get_distance() << endl;
            if (successor.get_distance() > successor_cost) {
                cout << "ADD SUCC" << endl;
                successor.set_distance(successor_cost);
                Arc origin = Arc(op, *state);
                successor.set_origin(&origin);
                queue.push(successor_cost, &successor);
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

AbstractState Abstraction::get_abstract_state(const State &state) const {
    // TODO: Make current a pointer?
    AbstractState current = single;
    while (!current.valid()) {
        int value = state[current.get_var()];
        current = *current.get_child(value);
    }
    assert(current.valid());
    return current;
}

void Abstraction::collect_states() {
    abs_states.clear();
    collect_child_states(&single);
}

void Abstraction::collect_child_states(AbstractState *parent) {
    if (parent->valid()) {
        abs_states.push_back(parent);
    } else {
        collect_child_states(parent->get_left_child());
        collect_child_states(parent->get_right_child());
    }
}

}

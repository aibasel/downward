#include "abstract_search.h"

#include "abstract_state.h"
#include "transition_system.h"
#include "utils.h"

#include "../utils/memory.h"

#include <cassert>

using namespace std;

namespace cegar {
AbstractSearch::AbstractSearch(
    const vector<int> &operator_costs)
    : operator_costs(operator_costs),
      search_info(1) {
}

void AbstractSearch::reset(int num_states) {
    open_queue.clear();
    search_info.resize(num_states);
    for (AbstractSearchInfo &info : search_info) {
        info.reset();
    }
}

unique_ptr<Solution> AbstractSearch::extract_solution(int init_id, int goal_id) const {
    unique_ptr<Solution> solution = utils::make_unique_ptr<Solution>();
    int current_id = goal_id;
    while (current_id != init_id) {
        const Transition &prev = search_info[current_id].get_incoming_transition();
        solution->emplace_front(prev.op_id, current_id);
        assert(prev.target_id != current_id);
        current_id = prev.target_id;
    }
    return solution;
}

void AbstractSearch::update_goal_distances(const Solution &solution) {
    /*
      Originally, we only updated the goal distances of states that are part of
      the trace (see Seipp and Helmert, JAIR 2018). The code below generalizes
      this idea and potentially updates the goal distances of all states.

      Let C* be the cost of the trace and g(s) be the g value of states s when
      A* finds the trace. Then for all states s with g(s) < INF (i.e., s has
      been reached by the search), C*-g(s) is a lower bound on the goal
      distance. This is the case since

      g(s) >= g*(s) [1]

      and

          f*(s) >= C*         (optimality of A* with an admissible heuristic)
      ==> g*(s) + h*(s) >= C* (definition of f values)
      ==> g(s) + h*(s) >= C*  (using [1])
      ==> h*(s) >= C* - g(s)  (arithmetic)

      Together with our existing lower bound h*(s) >= h(s), i.e., the h values
      from the last iteration, for each abstract state s with g(s) < INF, we
      can set h(s) = max(h(s), C*-g(s)).
    */
    int solution_cost = 0;
    for (const Transition &transition : solution) {
        solution_cost += operator_costs[transition.op_id];
    }
    for (auto &info : search_info) {
        if (info.get_g_value() < INF) {
            int new_h = max(info.get_h_value(), solution_cost - info.get_g_value());
            info.increase_h_value_to(new_h);
        }
    }
}

unique_ptr<Solution> AbstractSearch::find_solution(
    const vector<Transitions> &transitions,
    int init_id,
    const Goals &goal_ids) {
    reset(transitions.size());
    search_info[init_id].decrease_g_value_to(0);
    open_queue.push(search_info[init_id].get_h_value(), init_id);
    int goal_id = astar_search(transitions, goal_ids);
    open_queue.clear();
    bool has_found_solution = (goal_id != UNDEFINED);
    if (has_found_solution) {
        unique_ptr<Solution> solution = extract_solution(init_id, goal_id);
        update_goal_distances(*solution);
        return solution;
    } else {
        search_info[init_id].increase_h_value_to(INF);
    }
    return nullptr;
}

int AbstractSearch::astar_search(
    const vector<Transitions> &transitions, const Goals &goals) {
    while (!open_queue.empty()) {
        pair<int, int> top_pair = open_queue.pop();
        int old_f = top_pair.first;
        int state_id = top_pair.second;

        const int g = search_info[state_id].get_g_value();
        assert(0 <= g && g < INF);
        int new_f = g + search_info[state_id].get_h_value();
        assert(new_f <= old_f);
        if (new_f < old_f)
            continue;
        if (goals.count(state_id)) {
            return state_id;
        }
        assert(utils::in_bounds(state_id, transitions));
        for (const Transition &transition : transitions[state_id]) {
            int op_id = transition.op_id;
            int succ_id = transition.target_id;

            assert(utils::in_bounds(op_id, operator_costs));
            const int op_cost = operator_costs[op_id];
            assert(op_cost >= 0);
            int succ_g = (op_cost == INF) ? INF : g + op_cost;
            assert(succ_g >= 0);

            if (succ_g < search_info[succ_id].get_g_value()) {
                search_info[succ_id].decrease_g_value_to(succ_g);
                int h = search_info[succ_id].get_h_value();
                if (h == INF)
                    continue;
                int f = succ_g + h;
                assert(f >= 0);
                assert(f != INF);
                open_queue.push(f, succ_id);
                search_info[succ_id].set_incoming_transition(Transition(op_id, state_id));
            }
        }
    }
    return UNDEFINED;
}

int AbstractSearch::get_h_value(int state_id) const {
    assert(utils::in_bounds(state_id, search_info));
    return search_info[state_id].get_h_value();
}

void AbstractSearch::set_h_value(int state_id, int h) {
    assert(utils::in_bounds(state_id, search_info));
    search_info[state_id].increase_h_value_to(h);
}

void AbstractSearch::copy_h_value_to_children(int v, int v1, int v2) {
    int h = get_h_value(v);
    search_info.resize(search_info.size() + 1);
    set_h_value(v1, h);
    set_h_value(v2, h);
}


vector<int> compute_distances(
    const vector<Transitions> &transitions,
    const vector<int> &costs,
    const unordered_set<int> &start_ids) {
    vector<int> distances(transitions.size(), INF);
    priority_queues::AdaptiveQueue<int> open_queue;
    for (int goal_id : start_ids) {
        distances[goal_id] = 0;
        open_queue.push(0, goal_id);
    }
    while (!open_queue.empty()) {
        pair<int, int> top_pair = open_queue.pop();
        int old_g = top_pair.first;
        int state_id = top_pair.second;

        const int g = distances[state_id];
        assert(0 <= g && g < INF);
        assert(g <= old_g);
        if (g < old_g)
            continue;
        assert(utils::in_bounds(state_id, transitions));
        for (const Transition &transition : transitions[state_id]) {
            const int op_cost = costs[transition.op_id];
            assert(op_cost >= 0);
            int succ_g = (op_cost == INF) ? INF : g + op_cost;
            assert(succ_g >= 0);
            int succ_id = transition.target_id;
            if (succ_g < distances[succ_id]) {
                distances[succ_id] = succ_g;
                open_queue.push(succ_g, succ_id);
            }
        }
    }
    return distances;
}
}

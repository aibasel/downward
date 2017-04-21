#ifndef CEGAR_ABSTRACT_SEARCH_H
#define CEGAR_ABSTRACT_SEARCH_H

#include "transition.h"

#include "../algorithms/priority_queues.h"

#include <deque>
#include <unordered_set>
#include <vector>

namespace cegar {
class AbstractState;

using AbstractStates = std::unordered_set<AbstractState *>;
using Solution = std::deque<Transition>;

/*
  Find abstract solutions using A*. Compute g and h values for abstract
  states.
*/
class AbstractSearch {
    const std::vector<int> operator_costs;
    AbstractStates &states;

    priority_queues::AdaptiveQueue<AbstractState *> open_queue;
    Solution solution;

    void reset();

    void extract_solution(AbstractState *init, AbstractState *goal);

    AbstractState *astar_search(
        bool forward,
        bool use_h,
        AbstractStates *goals = nullptr);

public:
    AbstractSearch(
        std::vector<int> &&operator_costs,
        AbstractStates &states);

    bool find_solution(AbstractState *init, AbstractStates &goals);

    void forward_dijkstra(AbstractState *init);
    void backwards_dijkstra(const AbstractStates goals);

    const Solution &get_solution() {
        return solution;
    }
};
}

#endif

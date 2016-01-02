#ifndef CEGAR_ABSTRACT_SEARCH_H
#define CEGAR_ABSTRACT_SEARCH_H

#include "abstract_state.h"  // TODO: Remove include when Arc typedef is no longer needed.

#include "../priority_queue.h"
#include "../task_proxy.h"

#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

class Options;

namespace CEGAR {
using AbstractStates = std::unordered_set<AbstractState *>;
using Solution = std::deque<Arc>;

class AbstractSearch {
    const bool use_general_costs;

    AdaptiveQueue<AbstractState *> open_queue;
    std::unordered_map<AbstractState *, Arc> prev_arc;
    Solution solution;
    std::unordered_map<AbstractState *, int> g_values;

    void reset();

    void extract_solution(AbstractState *init, AbstractState *goal);

    AbstractState *astar_search(
        bool forward,
        bool use_h,
        AbstractStates *goals = nullptr,
        std::vector<int> *needed_costs = nullptr);

public:
    explicit AbstractSearch(const Options &opts);
    ~AbstractSearch() = default;

    bool find_solution(AbstractState *init, AbstractStates &goals);

    void backwards_dijkstra(const AbstractStates goals);

    /* Traverse abstraction and remember the minimum cost we need to keep for
       each operator in order not to decrease any heuristic values. */
    std::vector<int> get_needed_costs(AbstractState *init, int num_ops);

    const Solution &get_solution() {
        return solution;
    }

    int get_g_value(AbstractState *state) const;
};
}

#endif

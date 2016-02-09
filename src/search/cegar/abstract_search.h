#ifndef CEGAR_ABSTRACT_SEARCH_H
#define CEGAR_ABSTRACT_SEARCH_H

// TODO: Move include to .cc file when Arc typedef is no longer needed.
#include "abstract_state.h"

#include "../priority_queue.h"

#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace cegar {
using AbstractStates = std::unordered_set<AbstractState *>;
using Solution = std::deque<Arc>;

class AbstractSearch {
    const std::vector<int> operator_costs;
    const bool use_general_costs;

    AdaptiveQueue<AbstractState *> open_queue;
    /* TODO: Storing paths and g-values in unordered_maps is expensive.
       We should store them elsewhere. If we stick with unordered_maps,
       we should use one unordered_map for both infos. */
    std::unordered_map<AbstractState *, Arc> prev_arc;
    Solution solution;
    std::unordered_map<AbstractState *, int> g_values;

    void reset();

    void extract_solution(AbstractState *init, AbstractState *goal);

    // TODO: Replace use_h by lambda functions?
    AbstractState *astar_search(
        bool forward,
        bool use_h,
        AbstractStates *goals = nullptr,
        std::vector<int> *needed_costs = nullptr);

public:
    AbstractSearch(std::vector<int> &&operator_costs, bool use_general_costs);
    ~AbstractSearch() = default;

    bool find_solution(AbstractState *init, AbstractStates &goals);

    void backwards_dijkstra(const AbstractStates goals);

    /*
      Settle all nodes in the abstract transition system and remember
      the minimum cost we need to keep for each operator in order not
      to decrease any heuristic values.
    */
    std::vector<int> get_needed_costs(AbstractState *init, int num_ops);

    const Solution &get_solution() {
        return solution;
    }

    int get_g_value(AbstractState *state) const;
};
}

#endif

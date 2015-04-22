#ifndef CEGAR_ABSTRACT_SEARCH_H
#define CEGAR_ABSTRACT_SEARCH_H

#include "../priority_queue.h"
#include "../task_proxy.h"

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

class Options;

namespace cegar {
class AbstractState;

using AbstractStates = std::unordered_set<AbstractState *>;
using Arc = std::pair<OperatorProxy, AbstractState *>;
using Solution = std::unordered_map<AbstractState *, Arc>;

class AbstractSearch {
    const bool use_general_costs;

    AdaptiveQueue<AbstractState *> open_queue;
    Solution solution_backward;
    Solution solution_forward;
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

    std::vector<int> get_needed_costs(AbstractState *init, int num_ops);

    Solution &get_solution() {
        return solution_forward;
    }

    int get_g(AbstractState *state) const;
};
}

#endif

#ifndef HEURISTICS_BLIND_SEARCH_HEURISTIC_H
#define HEURISTICS_BLIND_SEARCH_HEURISTIC_H

#include "../heuristic.h"
#include "../tasks/root_task.h"

namespace blind_search_heuristic {
class BlindSearchHeuristic : public Heuristic {
    int min_operator_cost;
protected:
    virtual int compute_heuristic(const State &ancestor_state) override;
public:
    BlindSearchHeuristic(utils::Verbosity verbosity = utils::Verbosity::NORMAL,
                         const std::shared_ptr<AbstractTask> &transform = tasks::g_root_task,
                         bool cache_estimates = true);
    BlindSearchHeuristic(const plugins::Options &opts);
};
}

#endif

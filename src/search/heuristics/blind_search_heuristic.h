#ifndef HEURISTICS_BLIND_SEARCH_HEURISTIC_H
#define HEURISTICS_BLIND_SEARCH_HEURISTIC_H

#include "../heuristic.h"

namespace blind_search_heuristic {
using BlindSearchHeuristicArgs = WrapArgs<
    const std::shared_ptr<AbstractTask>, //&transform,
    bool, // cache_estimates,
    const std::string, //&description,
    utils::Verbosity // verbosity);
    >;
class BlindSearchHeuristic : public Heuristic {
    int min_operator_cost;
protected:
    virtual int compute_heuristic(const State &ancestor_state) override;
public:
    BlindSearchHeuristic(
        const std::shared_ptr<AbstractTask> &task,
        const std::shared_ptr<AbstractTask> &transform, bool cache_estimates,
        const std::string &description, utils::Verbosity verbosity);
};
}

#endif

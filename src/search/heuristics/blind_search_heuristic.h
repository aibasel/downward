#ifndef HEURISTICS_BLIND_SEARCH_HEURISTIC_H
#define HEURISTICS_BLIND_SEARCH_HEURISTIC_H

#include "../heuristic.h"

namespace blind_search_heuristic {
class BlindSearchHeuristic : public Heuristic {
    int min_operator_cost;
protected:
    virtual int compute_heuristic(const State &ancestor_state) override;
public:
    BlindSearchHeuristic(
        const std::shared_ptr<AbstractTask> &transform,
        bool cache_estimates, const std::string &description,
        utils::Verbosity verbosity);
};

class TaskIndependentBlindSearchHeuristic : public TaskIndependentHeuristic {
private:
    bool cache_evaluator_values;
    std::shared_ptr<Evaluator> create_task_specific(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map,
        int depth) const override;
public:
    TaskIndependentBlindSearchHeuristic(
        const std::shared_ptr</*TaskIndependent*/AbstractTask> transform,
        bool cache_estimates,
        const std::string &description,
        utils::Verbosity verbosity);

    virtual ~TaskIndependentBlindSearchHeuristic() override;

};


}

#endif

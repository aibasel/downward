#ifndef HEURISTICS_BLIND_SEARCH_HEURISTIC_H
#define HEURISTICS_BLIND_SEARCH_HEURISTIC_H

#include "../heuristic.h"

namespace blind_search_heuristic {
class BlindSearchHeuristic : public Heuristic {
    int min_operator_cost;
protected:
    virtual int compute_heuristic(const State &ancestor_state) override;
public:
    BlindSearchHeuristic(const std::string &name,
                         utils::Verbosity verbosity,
                         const std::shared_ptr<AbstractTask> task,
                         bool cache_evaluator_values);
    ~BlindSearchHeuristic();
};

class TaskIndependentBlindSearchHeuristic : public TaskIndependentHeuristic {
private:
    bool cache_evaluator_values;
public:
    explicit TaskIndependentBlindSearchHeuristic(const std::string &name,
                                                 utils::Verbosity verbosity,
                                                 std::shared_ptr<TaskIndependentAbstractTask> task_transformation,
                                                 bool cache_evaluator_values);

    virtual ~TaskIndependentBlindSearchHeuristic() override;

    std::shared_ptr<Evaluator>
    get_task_specific(const std::shared_ptr<AbstractTask> &task, std::unique_ptr<ComponentMap> &component_map,
                         int depth = -1) const override;

    std::shared_ptr<BlindSearchHeuristic> create_ts(
            const std::shared_ptr<AbstractTask> &task,
            std::unique_ptr<ComponentMap> &component_map,
            int depth) const;
};
}

#endif

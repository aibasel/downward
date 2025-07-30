#ifndef OPEN_LISTS_TIEBREAKING_OPEN_LIST_H
#define OPEN_LISTS_TIEBREAKING_OPEN_LIST_H

#include "../open_list_factory.h"


namespace tiebreaking_open_list {
class TieBreakingOpenListFactory : public OpenListFactory {
    std::vector<std::shared_ptr<Evaluator>> evals;
    bool unsafe_pruning;
    bool pref_only;
public:
    TieBreakingOpenListFactory(
        const std::vector<std::shared_ptr<Evaluator>> &evals,
        bool unsafe_pruning, bool pref_only);

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};



class TaskIndependentTieBreakingOpenListFactory : public TaskIndependentComponent<OpenListFactory> {
    bool pref_only;
    std::vector<std::shared_ptr<TaskIndependentComponent<Evaluator>>> evals;
    bool allow_unsafe_pruning;
    std::shared_ptr<OpenListFactory> create_task_specific(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map,
        int depth) const override;
public:
    explicit TaskIndependentTieBreakingOpenListFactory(
        std::vector<std::shared_ptr<TaskIndependentComponent<Evaluator>>> evals,
        bool pref_only,
        bool allow_unsafe_pruning);
    virtual ~TaskIndependentTieBreakingOpenListFactory() override = default;
};
}

#endif

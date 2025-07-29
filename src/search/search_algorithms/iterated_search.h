#ifndef SEARCH_ALGORITHMS_ITERATED_SEARCH_H
#define SEARCH_ALGORITHMS_ITERATED_SEARCH_H

#include "../search_algorithm.h"

#include "../parser/decorated_abstract_syntax_tree.h"

#include <memory>
#include <vector>

namespace iterated_search {
class IteratedSearch : public SearchAlgorithm {
    std::vector<std::shared_ptr<TaskIndependentComponent<SearchAlgorithm>>> search_algorithms;

    bool pass_bound;
    bool repeat_last_phase;
    bool continue_on_fail;
    bool continue_on_solve;

    int phase;
    bool last_phase_found_solution;
    int best_bound;
    bool iterated_found_solution;

    std::shared_ptr<SearchAlgorithm> get_search_algorithm(int algorithm_configs_index);
    std::shared_ptr<SearchAlgorithm> create_current_phase();
    SearchStatus step_return_value();

    virtual SearchStatus step() override;

public:
    IteratedSearch(
        std::vector<std::shared_ptr<TaskIndependentComponent<SearchAlgorithm>>> search_algorithms,
        bool pass_bound,
        bool repeat_last_phase,
        bool continue_on_fail,
        bool continue_on_solve,
        OperatorCost cost_type,
        int bound,
        double max_time,
        const std::string &name,
        utils::Verbosity verbosity,
        const std::shared_ptr<AbstractTask> &task);

    virtual void save_plan_if_necessary() override;
    virtual void print_statistics() const override;
};


class TaskIndependentIteratedSearch : public TaskIndependentComponent<SearchAlgorithm> {
private:
    int bound;
    OperatorCost cost_type;
    double max_time;
    std::vector<std::shared_ptr<TaskIndependentComponent<SearchAlgorithm>>> search_algorithms;

    bool pass_bound;
    bool repeat_last_phase;
    bool continue_on_fail;
    bool continue_on_solve;

    std::shared_ptr<SearchAlgorithm> create_task_specific(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map,
        int depth) const override;
public:
    TaskIndependentIteratedSearch(
        std::vector<std::shared_ptr<TaskIndependentComponent<SearchAlgorithm>>> search_algorithms,
        bool pass_bound,
        bool repeat_last_phase,
        bool continue_on_fail,
        bool continue_on_solve,
        OperatorCost cost_type,
        int bound,
        double max_time,
        const std::string &name,
        utils::Verbosity verbosity);

    virtual ~TaskIndependentIteratedSearch() override = default;


};



}

#endif

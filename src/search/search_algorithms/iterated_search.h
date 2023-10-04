#ifndef SEARCH_ALGORITHMS_ITERATED_SEARCH_H
#define SEARCH_ALGORITHMS_ITERATED_SEARCH_H

#include "../search_algorithm.h"

#include "../parser/decorated_abstract_syntax_tree.h"

#include <memory>
#include <vector>

namespace iterated_search {
class IteratedSearch : public SearchAlgorithm {
    std::vector<parser::LazyValue> algorithm_configs;

    bool pass_bound;
    bool repeat_last_phase;
    bool continue_on_fail;
    bool continue_on_solve;

    int phase;
    bool last_phase_found_solution;
    int best_bound;
    bool iterated_found_solution;

    std::shared_ptr<TaskIndependentSearchAlgorithm> get_search_algorithm(int algorithm_configs_index);
    std::shared_ptr<SearchAlgorithm> create_current_phase();
    SearchStatus step_return_value();

    virtual SearchStatus step() override;

public:
    IteratedSearch(const plugins::Options &opts);
    IteratedSearch(utils::Verbosity verbosity,
                   OperatorCost cost_type,
                   double max_time,
                   int bound,
                   const std::shared_ptr<AbstractTask> &task,
                   std::vector<parser::LazyValue> algorithm_configs,
                   bool pass_bound,
                   bool repeat_last_phase,
                   bool continue_on_fail,
                   bool continue_on_solve,
                   std::string unparsed_config = std::string());

    virtual void save_plan_if_necessary() override;
    virtual void print_statistics() const override;
};



class TaskIndependentIteratedSearch : public TaskIndependentSearchAlgorithm {
private:
    std::vector<parser::LazyValue> algorithm_configs;

    bool pass_bound;
    bool repeat_last_phase;
    bool continue_on_fail;
    bool continue_on_solve;
public:
    explicit TaskIndependentIteratedSearch(utils::Verbosity verbosity,
                                           OperatorCost cost_type,
                                           double max_time,
                                           std::string unparsed_config,
                                           std::vector<parser::LazyValue> algorithm_configs,
                                           bool pass_bound,
                                           bool repeat_last_phase,
                                           bool continue_on_fail,
                                           bool continue_on_solve);

    virtual std::shared_ptr<SearchAlgorithm> create_task_specific_SearchAlgorithm(
            const std::shared_ptr<AbstractTask> &task,
            std::shared_ptr<ComponentMap> &component_map, int depth = -1) override;

    virtual std::shared_ptr<IteratedSearch> create_task_specific_IteratedSearch(const std::shared_ptr<AbstractTask> &task, int depth = -1);
    virtual std::shared_ptr<IteratedSearch> create_task_specific_IteratedSearch(const std::shared_ptr<AbstractTask> &task, std::shared_ptr<ComponentMap> &component_map, int depth = -1);

    virtual ~TaskIndependentIteratedSearch()  override;
};


}

#endif

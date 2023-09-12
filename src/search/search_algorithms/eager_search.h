#ifndef SEARCH_ALGORITHMS_EAGER_SEARCH_H
#define SEARCH_ALGORITHMS_EAGER_SEARCH_H

#include "../open_list.h"
#include "../search_algorithm.h"

#include <memory>
#include <vector>

class Evaluator;
class TaskIndependentEvaluator;
class PruningMethod;

namespace plugins {
class Feature;
}

namespace eager_search {
class EagerSearch : public SearchAlgorithm {
    const bool reopen_closed_nodes;

    std::shared_ptr<StateOpenList> open_list;
    std::shared_ptr<Evaluator> f_evaluator;

    std::vector<Evaluator *> path_dependent_evaluators;
    std::vector<std::shared_ptr<Evaluator>> preferred_operator_evaluators;
    std::shared_ptr<Evaluator> lazy_evaluator;

    std::shared_ptr<PruningMethod> pruning_method;

    void start_f_value_statistics(EvaluationContext &eval_context);
    void update_f_value_statistics(EvaluationContext &eval_context);
    void reward_progress();

protected:
    virtual void initialize() override;
    virtual SearchStatus step() override;

public:
    explicit EagerSearch(const plugins::Options &opts);
    explicit EagerSearch(utils::Verbosity verbosity,
                         OperatorCost cost_type,
                         double max_time,
                         int bound,
                         bool reopen_closed_nodes,
                         std::shared_ptr<StateOpenList> open_list,
                         std::vector<std::shared_ptr<Evaluator>> preferred_operator_evaluators,
                         std::shared_ptr<PruningMethod> pruning_method,
                         std::shared_ptr<AbstractTask> &task,
                         std::shared_ptr<Evaluator> f_evaluator = nullptr,
                         std::shared_ptr<Evaluator> lazy_evaluator = nullptr,
                         std::string unparsed_config = std::string());
    virtual ~EagerSearch() = default;

    virtual void print_statistics() const override;

    void dump_search_space() const;
};



class TaskIndependentEagerSearch : public TaskIndependentSearchEngine {
private:
    const bool reopen_closed_nodes;

    std::shared_ptr<TaskIndependentStateOpenList> open_list;
    std::shared_ptr<TaskIndependentEvaluator> f_evaluator;

    std::vector<TaskIndependentEvaluator *> path_dependent_evaluators;
    std::vector<std::shared_ptr<TaskIndependentEvaluator>> preferred_operator_evaluators;
    std::shared_ptr<TaskIndependentEvaluator> lazy_evaluator;

    std::shared_ptr<PruningMethod> pruning_method;
public:
    explicit TaskIndependentEagerSearch(utils::Verbosity verbosity,
                                        OperatorCost cost_type,
                                        double max_time,
                                        int bound,
                                        bool reopen_closed_nodes,
                                        std::shared_ptr<TaskIndependentStateOpenList> open_list,
                                        std::vector<std::shared_ptr<TaskIndependentEvaluator>> preferred_operator_evaluators,
                                        std::shared_ptr<PruningMethod> pruning_method,
                                        std::shared_ptr<TaskIndependentEvaluator> f_evaluator = nullptr,
                                        std::shared_ptr<TaskIndependentEvaluator> lazy_evaluator = nullptr,
                                        std::string unparsed_config = std::string());

    plugins::Any create_task_specific(std::shared_ptr<AbstractTask> &task, std::shared_ptr<ComponentMap> &component_map) override;

    virtual ~TaskIndependentEagerSearch()  override;
};


extern void add_options_to_feature(plugins::Feature &feature);
}

#endif

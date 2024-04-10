#ifndef SEARCH_ALGORITHMS_EAGER_SEARCH_H
#define SEARCH_ALGORITHMS_EAGER_SEARCH_H

#include "../open_list.h"
#include "../search_algorithm.h"

#include <memory>
#include <vector>

class Evaluator;
class PruningMethod;
class OpenListFactory;

namespace plugins {
class Feature;
}

namespace eager_search {
class EagerSearch : public SearchAlgorithm {
    const bool reopen_closed_nodes;

    std::unique_ptr<StateOpenList> open_list;
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
    explicit EagerSearch(
        const std::shared_ptr<OpenListFactory> &open,
        bool reopen_closed, const std::shared_ptr<Evaluator> &f_eval,
        const std::vector<std::shared_ptr<Evaluator>> &preferred,
        const std::shared_ptr<PruningMethod> &pruning,
        const std::shared_ptr<Evaluator> &lazy_evaluator,
        OperatorCost cost_type, int bound, double max_time,
        const std::string &description, utils::Verbosity verbosity);

    virtual void print_statistics() const override;

    void dump_search_space() const;
};

extern void add_eager_search_options_to_feature(
    plugins::Feature &feature, const std::string &description);
extern std::tuple<std::shared_ptr<PruningMethod>,
                  std::shared_ptr<Evaluator>, OperatorCost, int, double,
                  std::string, utils::Verbosity>
get_eager_search_arguments_from_options(const plugins::Options &opts);
}

#endif

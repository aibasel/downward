#ifndef EVALUATORS_G_EVALUATOR_H
#define EVALUATORS_G_EVALUATOR_H

#include "../abstract_task.h"
#include "../evaluator.h"

namespace g_evaluator {
class GEvaluator : public Evaluator {
public:
    explicit GEvaluator(const plugins::Options &opts);
    explicit GEvaluator(const std::string &name,
                        utils::Verbosity verbosity);
    virtual ~GEvaluator() override = default;

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;

    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &) override {}
};


class TaskIndependentGEvaluator : public TaskIndependentEvaluator {
public:
    explicit TaskIndependentGEvaluator(const std::string &name,
                                       utils::Verbosity verbosity);

    virtual ~TaskIndependentGEvaluator()  override = default;


    std::shared_ptr<Evaluator>
    create_task_specific(const std::shared_ptr<AbstractTask> &task, std::unique_ptr<ComponentMap> &component_map,
                         int depth = -1) const override;
};
}

#endif

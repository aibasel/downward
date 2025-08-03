#ifndef EVALUATORS_G_EVALUATOR_H
#define EVALUATORS_G_EVALUATOR_H

#include "../evaluator.h"

namespace g_evaluator {
using GEvaluatorArgs = ComponentArgs;
class GEvaluator : public Evaluator {
public:
    GEvaluator(
        const std::shared_ptr<AbstractTask> &task,
        const std::string &description, utils::Verbosity verbosity);

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;

    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &) override {}
};

//using TaskIndepenendGEvaluatorArgs = InnerTaskIndependnet_t<GEvaluatorArgs>;
//
//class TaskIndependentGEvaluator : public TaskIndependentComponent<Evaluator> {
//    std::shared_ptr<Evaluator> create_task_specific(
//        const std::shared_ptr<AbstractTask> &task,
//        std::unique_ptr<ComponentMap> &component_map,
//        int depth) const override;
//public:
//    TaskIndependentGEvaluator(TaskIndepenendGEvaluatorArgs args);
//    TaskIndependentGEvaluator(const std::string &description,
//                              utils::Verbosity verbosity);
//
//    virtual ~TaskIndependentGEvaluator()  override = default;
//};
}

#endif

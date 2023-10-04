#ifndef EVALUATORS_G_EVALUATOR_H
#define EVALUATORS_G_EVALUATOR_H

#include "../abstract_task.h"
#include "../evaluator.h"

namespace g_evaluator {
class GEvaluator : public Evaluator {
public:
    explicit GEvaluator(const plugins::Options &opts);
    explicit GEvaluator(utils::LogProxy log,
                        std::basic_string<char> unparsed_config = std::string(),
                        bool use_for_reporting_minima = false,
                        bool use_for_boosting = false,
                        bool use_for_counting_evaluations = false);
    virtual ~GEvaluator() override = default;

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;

    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &) override {}
};


class TaskIndependentGEvaluator : public TaskIndependentEvaluator {
private:
    std::string unparsed_config;
    utils::LogProxy log;
public:
    explicit TaskIndependentGEvaluator(utils::LogProxy log,
                                       std::string unparsed_config = std::string(),
                                       bool use_for_reporting_minima = false,
                                       bool use_for_boosting = false,
                                       bool use_for_counting_evaluations = false);
    template<typename T>
    std::shared_ptr<T> create_task_specific(std::shared_ptr<AbstractTask> &task, std::shared_ptr<ComponentMap> &component_map, int depth = -1);

    virtual ~TaskIndependentGEvaluator()  override;


    virtual std::shared_ptr<Evaluator> create_task_specific_Evaluator(
        std::shared_ptr<AbstractTask> &task,
        std::shared_ptr<ComponentMap> &component_map, int depth = -1) override;

    virtual std::shared_ptr<GEvaluator> create_task_specific_GEvaluator(std::shared_ptr<AbstractTask> &task, int depth = -1);
    virtual std::shared_ptr<GEvaluator> create_task_specific_GEvaluator(std::shared_ptr<AbstractTask> &task, std::shared_ptr<ComponentMap> &component_map, int depth = -1);
};
}

#endif

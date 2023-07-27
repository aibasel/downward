
#ifndef DOWNWARD_TASK_INDEPENDENT_EVALUATOR_H
#define DOWNWARD_TASK_INDEPENDENT_EVALUATOR_H

#include "abstract_task.h"
#include "evaluator.h"

#include "utils/logging.h"

class TaskIndependentEvaluator {
    const std::string description;
    const bool use_for_reporting_minima;
    const bool use_for_boosting;
    const bool use_for_counting_evaluations;
protected:
    mutable utils::LogProxy log;
public:
    explicit TaskIndependentEvaluator(
            utils::LogProxy log,
            const std::string unparsed_config = std::string(),
            bool use_for_reporting_minima = false,
            bool use_for_boosting = false,
            bool use_for_counting_evaluations = false);
    virtual ~TaskIndependentEvaluator() = default;

    virtual std::shared_ptr<Evaluator> create_task_specific(std::shared_ptr<AbstractTask> &task) = 0;
};



#endif //DOWNWARD_TASK_INDEPENDENT_EVALUATOR_H

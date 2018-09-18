#ifndef EVALUATORS_PREF_EVALUATOR_H
#define EVALUATORS_PREF_EVALUATOR_H

#include "../evaluator.h"

#include <string>
#include <vector>

namespace pref_evaluator {
class PrefEvaluator : public Evaluator {
public:
    PrefEvaluator();
    virtual ~PrefEvaluator() override;

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;
    virtual void get_path_dependent_evaluators(std::set<std::shared_ptr<Evaluator>> &) override {}
    //TODO: remove when all search algorithms use shared_ptr for plugins
    virtual void get_path_dependent_evaluators(std::set<Evaluator *> &) override {}
};
}

#endif

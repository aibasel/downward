#ifndef EVALUATORS_PREF_EVALUATOR_H
#define EVALUATORS_PREF_EVALUATOR_H

#include "../scalar_evaluator.h"

#include <string>
#include <vector>


namespace PrefEvaluator {
class PrefEvaluator : public ScalarEvaluator {
public:
    PrefEvaluator();
    virtual ~PrefEvaluator() override;

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;
    virtual void get_involved_heuristics(std::set<Heuristic *> &) override {}
};
}

#endif

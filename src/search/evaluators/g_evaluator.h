#ifndef EVALUATORS_G_EVALUATOR_H
#define EVALUATORS_G_EVALUATOR_H

#include "../scalar_evaluator.h"

class Heuristic;


namespace GEvaluator {
class GEvaluator : public ScalarEvaluator {
public:
    GEvaluator() = default;
    virtual ~GEvaluator() override = default;

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;

    virtual void get_involved_heuristics(std::set<Heuristic *> &) override {}
};
}

#endif

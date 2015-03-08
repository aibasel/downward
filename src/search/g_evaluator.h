#ifndef G_EVALUATOR_H
#define G_EVALUATOR_H

#include "scalar_evaluator.h"

class Heuristic;

class GEvaluator : public ScalarEvaluator {
public:
    GEvaluator();
    virtual ~GEvaluator() override;

    virtual bool dead_end_is_reliable() const override;
    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;
    virtual void get_involved_heuristics(std::set<Heuristic *> &) override {}
};

#endif

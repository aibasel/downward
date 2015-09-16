#ifndef WEIGHTED_EVALUATOR_H
#define WEIGHTED_EVALUATOR_H

#include "scalar_evaluator.h"

class Options;

class WeightedEvaluator : public ScalarEvaluator {
    ScalarEvaluator *evaluator;
    int w;

public:
    explicit WeightedEvaluator(const Options &opts);
    WeightedEvaluator(ScalarEvaluator *eval, int weight);
    virtual ~WeightedEvaluator() override;

    virtual bool dead_ends_are_reliable() const override;
    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;
    virtual void get_involved_heuristics(std::set<Heuristic *> &hset) override;
};

#endif

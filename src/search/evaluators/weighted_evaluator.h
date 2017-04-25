#ifndef EVALUATORS_WEIGHTED_EVALUATOR_H
#define EVALUATORS_WEIGHTED_EVALUATOR_H

#include "../evaluator.h"

namespace options {
class Options;
}

namespace weighted_evaluator {
class WeightedEvaluator : public Evaluator {
    Evaluator *evaluator;
    int w;

public:
    explicit WeightedEvaluator(const options::Options &opts);
    WeightedEvaluator(Evaluator *eval, int weight);
    virtual ~WeightedEvaluator() override;

    virtual bool dead_ends_are_reliable() const override;
    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;
    virtual void get_involved_heuristics(std::set<Heuristic *> &hset) override;
};
}

#endif

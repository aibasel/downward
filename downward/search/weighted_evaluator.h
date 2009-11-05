#ifndef WEIGHTED_EVALUATOR_H
#define WEIGHTED_EVALUATOR_H

#include "scalar_evaluator.h"

class WeightedEvaluator : public ScalarEvaluator {
private:
    ScalarEvaluator* evaluator;
    int w;
    int value;

public:
    WeightedEvaluator(ScalarEvaluator *eval, int weight);
    ~WeightedEvaluator();

    void evaluate(int g, bool preferred);
    bool is_dead_end() const;
    bool dead_end_is_reliable() const;
    int get_value() const;
};

#endif



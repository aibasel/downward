#ifndef WEIGHTED_EVALUATOR_H
#define WEIGHTED_EVALUATOR_H

#include "scalar_evaluator.h"

#include <string>
#include <vector>

class Options;

class WeightedEvaluator : public ScalarEvaluator {
private:
    ScalarEvaluator *evaluator;
    int w;
    int value;

public:
    WeightedEvaluator(const Options &opts);
    WeightedEvaluator(ScalarEvaluator *eval, int weight);
    ~WeightedEvaluator();

    void evaluate(int g, bool preferred);
    bool is_dead_end() const;
    bool dead_end_is_reliable() const;
    int get_value() const;
    void get_involved_heuristics(std::set<Heuristic *> &hset);
};

#endif

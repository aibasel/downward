#ifndef SUM_EVALUATOR_H
#define SUM_EVALUATOR_H

#include "scalar_evaluator.h"

#include <string>
#include <vector>

class Options;

class SumEvaluator : public ScalarEvaluator {
private:
    std::vector<ScalarEvaluator *> evaluators;
    int value;
    bool dead_end;
    bool dead_end_reliable;

public:
    SumEvaluator(const Options &opts);
    SumEvaluator(const std::vector<ScalarEvaluator *> &evals);
    ~SumEvaluator();

    void evaluate(int g, bool preferred);
    bool is_dead_end() const;
    bool dead_end_is_reliable() const;
    int get_value() const;
    void get_involved_heuristics(std::set<Heuristic *> &hset);

};

#endif

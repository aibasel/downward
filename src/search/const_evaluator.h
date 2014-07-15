#ifndef CONST_EVALUATOR_H
#define CONST_EVALUATOR_H

#include "scalar_evaluator.h"
#include <string>
#include <vector>

class Heuristic;
class Options;

class ConstEvaluator : public ScalarEvaluator {
private:
    int value;

public:
    ConstEvaluator(const Options &opts);
    ConstEvaluator(const int value);
    ~ConstEvaluator();

    void evaluate(int, bool) {}
    bool is_dead_end() const;
    bool dead_end_is_reliable() const;
    int get_value() const;
    void get_involved_heuristics(std::set<Heuristic *> &) {}

};

#endif

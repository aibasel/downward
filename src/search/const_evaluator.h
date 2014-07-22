#ifndef CONST_EVALUATOR_H
#define CONST_EVALUATOR_H

#include "heuristic.h"
#include <string>
#include <vector>

class Options;

class ConstEvaluator : public Heuristic {
private:
    int value;

protected:
    int compute_heuristic(const State &state);

public:
    ConstEvaluator(const Options &opts);
    ~ConstEvaluator();

    void evaluate(int, bool) {}
    bool is_dead_end() const;
    bool dead_end_is_reliable() const;



};

#endif

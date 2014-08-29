#ifndef CONST_EVALUATOR_H
#define CONST_EVALUATOR_H

#include "heuristic.h"

#include <string>
#include <vector>

class Options;

class ConstEvaluator : public Heuristic {
    int value;

protected:
    virtual int compute_heuristic(const State &state);

public:
    ConstEvaluator(const Options &opts);
    virtual ~ConstEvaluator();

    virtual void evaluate(int, bool) {}
    virtual bool is_dead_end() const;
    virtual bool dead_end_is_reliable() const;
};

#endif

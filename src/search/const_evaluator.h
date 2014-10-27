#ifndef CONST_EVALUATOR_H
#define CONST_EVALUATOR_H

#include "heuristic.h"

class Options;

// TODO: as soon as search doesn't need at least one heuristic this should inherit from ScalarEvaluator
class ConstEvaluator : public Heuristic {
    int value;

protected:
    virtual int compute_heuristic(const GlobalState &);

public:
    explicit ConstEvaluator(const Options &opts);
    virtual ~ConstEvaluator();

    virtual void evaluate(int, bool) {}
    virtual bool is_dead_end() const;
    virtual bool dead_end_is_reliable() const;
};

#endif

#ifndef CONST_EVALUATOR_H
#define CONST_EVALUATOR_H

#include "heuristic.h"

#include <string>
#include <vector>

class Options;

// TODO: as soon as search doesn't need atleast one heuristic this should be converted to a ScalarEvaluator
class ConstEvaluator : public Heuristic {
    int value;

protected:
    virtual int compute_heuristic(const GlobalState &);

public:
    ConstEvaluator(const Options &opts);
    virtual ~ConstEvaluator();

    virtual void evaluate(int, bool) {}
    virtual bool is_dead_end() const;
    virtual bool dead_end_is_reliable() const;
};

#endif

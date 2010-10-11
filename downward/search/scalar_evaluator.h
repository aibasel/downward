#ifndef SCALAR_EVALUATOR_H
#define SCALAR_EVALUATOR_H

#include "evaluator.h"

class ScalarEvaluator : public Evaluator {
public:
    virtual ~ScalarEvaluator() {}

    virtual int get_value() const = 0;
};

#endif

#ifndef EVALUATORS_CONST_EVALUATOR_H
#define EVALUATORS_CONST_EVALUATOR_H

#include "../heuristic.h"

namespace const_evaluator {
// TODO: When the searches don't need at least one heuristic anymore,
//       ConstEvaluator should inherit from Evaluator.
class ConstEvaluator : public Heuristic {
    int value;

protected:
    virtual int compute_heuristic(const GlobalState &) override;

public:
    explicit ConstEvaluator(const options::Options &opts);
    virtual ~ConstEvaluator() override = default;
};
}

#endif

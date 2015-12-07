#ifndef EVALUATORS_CONST_EVALUATOR_H
#define EVALUATORS_CONST_EVALUATOR_H

#include "../heuristic.h"

namespace ConstEvaluator {
// TODO: When the searches don't need at least one heuristic anymore,
//       ConstEvaluator should inherit from ScalarEvaluator.
class ConstEvaluator : public Heuristic {
    int value;

protected:
    virtual int compute_heuristic(const GlobalState &);

public:
    explicit ConstEvaluator(const Options &opts);
    virtual ~ConstEvaluator() override = default;
};
}

#endif

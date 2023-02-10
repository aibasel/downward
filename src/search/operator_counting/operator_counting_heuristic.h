#ifndef OPERATOR_COUNTING_OPERATOR_COUNTING_HEURISTIC_H
#define OPERATOR_COUNTING_OPERATOR_COUNTING_HEURISTIC_H

#include "../heuristic.h"

#include "../lp/lp_solver.h"

#include <memory>
#include <vector>

namespace plugins {
class Options;
}

namespace operator_counting {
class ConstraintGenerator;

class OperatorCountingHeuristic : public Heuristic {
    std::vector<std::shared_ptr<ConstraintGenerator>> constraint_generators;
    lp::LPSolver lp_solver;
    const bool use_integer_operator_counts;
protected:
    virtual int compute_heuristic(const State &ancestor_state) override;
public:
    explicit OperatorCountingHeuristic(const plugins::Options &opts);
    ~OperatorCountingHeuristic();
};
}

#endif

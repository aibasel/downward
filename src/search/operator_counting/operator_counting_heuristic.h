#ifndef OPERATOR_COUNTING_OPERATOR_COUNTING_HEURISTIC_H
#define OPERATOR_COUNTING_OPERATOR_COUNTING_HEURISTIC_H

#include "../heuristic.h"

#include "../lp/lp_solver.h"

#include <memory>
#include <vector>

namespace options {
class Options;
}

namespace operator_counting {
class ConstraintGenerator;

class OperatorCountingHeuristic : public Heuristic {
    std::vector<std::shared_ptr<ConstraintGenerator>> constraint_generators;
    lp::LPSolver lp_solver;
protected:
    virtual int compute_heuristic(const GlobalState &global_state) override;
    int compute_heuristic(const State &state);
public:
    explicit OperatorCountingHeuristic(const options::Options &opts);
    ~OperatorCountingHeuristic();
};
}

#endif

#ifndef OPERATOR_COUNTING_OPERATOR_COUNTING_HEURISTIC_H
#define OPERATOR_COUNTING_OPERATOR_COUNTING_HEURISTIC_H


#include "../heuristic.h"
#include "../lp_solver.h"

#include <memory>
#include <vector>

class Options;

namespace OperatorCounting {
class ConstraintGenerator;

class OperatorCountingHeuristic : public Heuristic {
    std::vector<std::shared_ptr<ConstraintGenerator>> constraint_generators;
    LPSolver lp_solver;
protected:
    virtual void initialize() override;
    virtual int compute_heuristic(const GlobalState &global_state) override;
    int compute_heuristic(const State &state);
public:
    explicit OperatorCountingHeuristic(const Options &opts);
    ~OperatorCountingHeuristic();
};
}

#endif

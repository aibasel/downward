#ifndef OPERATOR_COUNTING_OPERATOR_COUNTING_HEURISTIC_H
#define OPERATOR_COUNTING_OPERATOR_COUNTING_HEURISTIC_H


#include "../heuristic.h"
#include "../lp_solver.h"

#include <vector>

class Options;

namespace operator_counting {
class ConstraintGenerator;

class OperatorCountingHeuristic : public Heuristic {
    std::vector<ConstraintGenerator *> constraint_generators;
    LPSolver lp_solver;
protected:
    virtual void initialize() override;
    virtual bool reach_state(const GlobalState &parent_state,
                             const GlobalOperator &op,
                             const GlobalState &state) override;
    virtual int compute_heuristic(const GlobalState &state) override;
public:
    OperatorCountingHeuristic(const Options &opts);
    ~OperatorCountingHeuristic();
};
}

#endif

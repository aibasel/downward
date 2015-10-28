#ifndef OPERATOR_COUNTING_CONSTRAINT_GENERATOR_H
#define OPERATOR_COUNTING_CONSTRAINT_GENERATOR_H

#include <memory>
#include <vector>

class AbstractTask;
class LPConstraint;
class LPSolver;
class State;

namespace operator_counting {
class ConstraintGenerator {
public:
    /*
      Add permanent constraints.
    */
    virtual void initialize_constraints(
        const std::shared_ptr<AbstractTask> /*task*/,
        std::vector<LPConstraint> & /*constraints*/,
        double /*infinity*/) {
    }

    /*
      Set bounds on permanent constraints or add temporary constraints.
      Returns true if a dead end was detected and false otherwise.
    */
    virtual bool update_constraints(const State &state,
                                    LPSolver &lp_solver) = 0;
};
}

#endif

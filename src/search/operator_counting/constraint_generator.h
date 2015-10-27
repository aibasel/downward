#ifndef OPERATOR_COUNTING_CONSTRAINT_GENERATOR_H
#define OPERATOR_COUNTING_CONSTRAINT_GENERATOR_H

#include "../global_operator.h"
#include "../global_state.h"

#include <vector>

class LPConstraint;
class LPSolver;
class Options;

namespace operator_counting {

class ConstraintGenerator {
public:
    /*
      Add permanent constraints.
    */
    virtual void initialize_constraints(
        std::vector<LPConstraint> &/*constraints*/) {
    }

    /*
      This method can be used to inform heuristics that are part of the
      ConstraintGenerator that a new state was reached.
    */
    // TODO In the long term, we should use the existing mechanisms here.
    //      Heuristics should be collected with get_involved_heuristics and
    //      then evaluated by the search algorithm.
    //      Currently this is not possible, because the evaluations are not
    //      necessarily evaluated in the correct order.
    virtual bool reach_state(const GlobalState & /*parent_state*/,
                             const GlobalOperator & /*op*/,
                             const GlobalState & /*state*/) {
        return false;
    }

    /*
      Set bounds on permanent constraints or add temporary constraints.
      Returns true if a dead end was detected and false otherwise.
    */
    virtual bool update_constraints(const GlobalState &state,
                                    LPSolver &lp_solver) = 0;
};
}

#endif

#ifndef OPERATOR_COUNTING_STATE_EQUATION_CONSTRAINTS_H
#define OPERATOR_COUNTING_STATE_EQUATION_CONSTRAINTS_H

#include "constraint_generator.h"

#include <set>

class TaskProxy;

namespace lp {
class LPConstraint;
}

namespace operator_counting {
/* A proposition is an atom of the form Var = Val. It stores the index of the
   constraint representing it in the LP */
struct Proposition {
    int constraint_index;
    std::set<int> always_produced_by;
    std::set<int> sometimes_produced_by;
    std::set<int> always_consumed_by;

    Proposition() : constraint_index(-1) {
    }
    ~Proposition() = default;
};

class StateEquationConstraints : public ConstraintGenerator {
    std::vector<std::vector<Proposition>> propositions;
    // Map goal variables to their goal value and other variables to max int.
    std::vector<int> goal_state;

    void build_propositions(const TaskProxy &task_proxy);
    void add_constraints(named_vector::NamedVector<lp::LPConstraint> &constraints, double infinity);
public:
    virtual void initialize_constraints(const std::shared_ptr<AbstractTask> &task,
                                        named_vector::NamedVector<lp::LPConstraint> &constraints,
                                        double infinity) override;
    virtual bool update_constraints(const State &state, lp::LPSolver &lp_solver) override;
};
}

#endif

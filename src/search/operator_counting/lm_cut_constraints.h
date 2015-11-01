#ifndef OPERATOR_COUNTING_LM_CUT_CONSTRAINTS_H
#define OPERATOR_COUNTING_LM_CUT_CONSTRAINTS_H

#include  "constraint_generator.h"

#include <memory>

class LandmarkCutLandmarks;

namespace OperatorCounting {
class LMCutConstraints : public ConstraintGenerator {
    std::unique_ptr<LandmarkCutLandmarks> landmark_generator;
public:
    virtual void initialize_constraints(
        const std::shared_ptr<AbstractTask> task,
        std::vector<LPConstraint> &constraints,
        double infinity) override;
    virtual bool update_constraints(const State &state,
                                    LPSolver &lp_solver) override;
};
}

#endif

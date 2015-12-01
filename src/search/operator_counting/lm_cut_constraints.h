#ifndef OPERATOR_COUNTING_LM_CUT_CONSTRAINTS_H
#define OPERATOR_COUNTING_LM_CUT_CONSTRAINTS_H

#include  "constraint_generator.h"

#include <memory>

namespace LandmarkCutHeuristic {
class LandmarkCutLandmarks;
}


namespace OperatorCounting {
class LMCutConstraints : public ConstraintGenerator {
    std::unique_ptr<LandmarkCutHeuristic::LandmarkCutLandmarks> landmark_generator;
public:
    virtual void initialize_constraints(
        const std::shared_ptr<AbstractTask> task,
        std::vector<LP::LPConstraint> &constraints,
        double infinity) override;
    virtual bool update_constraints(const State &state,
                                    LP::LPSolver &lp_solver) override;
};
}

#endif

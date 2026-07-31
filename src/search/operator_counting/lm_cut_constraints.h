#ifndef OPERATOR_COUNTING_LM_CUT_CONSTRAINTS_H
#define OPERATOR_COUNTING_LM_CUT_CONSTRAINTS_H

#include "constraint_generator.h"

#include <memory>

namespace lm_cut_heuristic {
class LandmarkCutLandmarks;
}

namespace operator_counting {
class LMCutConstraints : public ConstraintGenerator {
    bool use_goal_zone_detection;
    bool use_border_detection;
    std::unique_ptr<lm_cut_heuristic::LandmarkCutLandmarks> landmark_generator;
public:
    LMCutConstraints(
        const std::shared_ptr<AbstractTask> &task, bool use_goal_zone_detection,
        bool use_border_detection);
    virtual void initialize_constraints(
        const std::shared_ptr<AbstractTask> &task,
        lp::LinearProgram &lp) override;
    virtual bool update_constraints(
        const State &state, lp::LPSolver &lp_solver) override;
};
}

#endif

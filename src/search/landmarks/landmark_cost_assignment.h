#ifndef LANDMARKS_LANDMARK_COST_ASSIGNMENT_H
#define LANDMARKS_LANDMARK_COST_ASSIGNMENT_H

#include "../lp/lp_solver.h"

#include <set>
#include <vector>

class OperatorsProxy;

namespace landmarks {
class Landmark;
class LandmarkGraph;
class LandmarkNode;
class LandmarkStatusManager;

class LandmarkCostAssignment {
    const std::set<int> empty;
protected:
    const LandmarkGraph &lm_graph;
    const std::vector<int> operator_costs;

    const std::set<int> &get_achievers(int lmn_status,
                                       const Landmark &landmark) const;
public:
    LandmarkCostAssignment(const std::vector<int> &operator_costs,
                           const LandmarkGraph &graph);
    virtual ~LandmarkCostAssignment() = default;

    virtual double cost_sharing_h_value(
        const LandmarkStatusManager &lm_status_manager) = 0;
};

class LandmarkUniformSharedCostAssignment : public LandmarkCostAssignment {
    bool use_action_landmarks;
public:
    LandmarkUniformSharedCostAssignment(const std::vector<int> &operator_costs,
                                        const LandmarkGraph &graph,
                                        bool use_action_landmarks);

    virtual double cost_sharing_h_value(
        const LandmarkStatusManager &lm_status_manager) override;
};

class LandmarkEfficientOptimalSharedCostAssignment : public LandmarkCostAssignment {
    lp::LPSolver lp_solver;
    // We keep an additional copy of the constraints around to avoid some effort with recreating the vector (see issue443).
    std::vector<lp::LPConstraint> lp_constraints;
    /*
      We keep the vectors for LP variables and constraints around instead of
      recreating them for every state. The actual constraints have to be
      recreated because the coefficient matrix of the LP changes from state to
      state. Reusing the vectors still saves some dynamic allocation overhead.
     */
    lp::LinearProgram lp;

    lp::LinearProgram build_initial_lp();
public:
    LandmarkEfficientOptimalSharedCostAssignment(
        const std::vector<int> &operator_costs,
        const LandmarkGraph &graph,
        lp::LPSolverType solver_type);

    virtual double cost_sharing_h_value(
        const LandmarkStatusManager &lm_status_manager) override;
};
}

#endif

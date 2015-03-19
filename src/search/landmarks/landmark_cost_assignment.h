#ifndef LANDMARKS_LANDMARK_COST_ASSIGNMENT_H
#define LANDMARKS_LANDMARK_COST_ASSIGNMENT_H

#include "../globals.h"
#include "../lp_solver.h"
#include "../operator_cost.h"

#include <set>
#include <vector>

class LandmarkGraph;
class LandmarkNode;

class LandmarkCostAssignment {
    const std::set<int> empty;
protected:
    LandmarkGraph &lm_graph;
    OperatorCost cost_type;

    const std::set<int> &get_achievers(int lmn_status,
                                       const LandmarkNode &lmn) const;
public:
    LandmarkCostAssignment(LandmarkGraph &graph, OperatorCost cost_type_);
    virtual ~LandmarkCostAssignment();

    virtual double cost_sharing_h_value() = 0;
};

class LandmarkUniformSharedCostAssignment : public LandmarkCostAssignment {
    bool use_action_landmarks;
public:
    LandmarkUniformSharedCostAssignment(LandmarkGraph &graph, bool use_action_landmarks_, OperatorCost cost_type_);
    virtual ~LandmarkUniformSharedCostAssignment();

    virtual double cost_sharing_h_value();
};

class LandmarkEfficientOptimalSharedCostAssignment : public LandmarkCostAssignment {
    LPSolver lp_solver;
    /*
      We keep the vectors for LP variables and constraints around instead of
      recreating them for every state. The actual constraints have to be
      recreated because the coefficient matrix of the LP changes from state to
      state. Reusing the vectors still saves some dynamic allocation overhead.
     */
    std::vector<LPVariable> lp_variables;
    std::vector<LPConstraint> lp_constraints;
    std::vector<LPConstraint> non_empty_lp_constraints;
public:
    LandmarkEfficientOptimalSharedCostAssignment(LandmarkGraph &graph, OperatorCost cost_type, LPSolverType solver_type);
    virtual ~LandmarkEfficientOptimalSharedCostAssignment();

    virtual double cost_sharing_h_value();
};


#endif

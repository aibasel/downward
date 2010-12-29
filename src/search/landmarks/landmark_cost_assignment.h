#ifndef LANDMARKS_LANDMARK_COST_ASSIGNMENT_H
#define LANDMARKS_LANDMARK_COST_ASSIGNMENT_H

#include <set>
#include "../globals.h"

class LandmarksGraph;
class LandmarkNode;

class LandmarkCostAssignment {
    const std::set<int> empty;
protected:
    LandmarksGraph &lm_graph;
    OperatorCost cost_type;

    const std::set<int> &get_achievers(int lmn_status,
                                       const LandmarkNode &lmn) const;
public:
    explicit LandmarkCostAssignment(LandmarksGraph &graph, OperatorCost cost_type_);
    virtual ~LandmarkCostAssignment();

    virtual double cost_sharing_h_value() = 0;
};

class LandmarkUniformSharedCostAssignment : public LandmarkCostAssignment {
    bool use_action_landmarks;
public:
    LandmarkUniformSharedCostAssignment(LandmarksGraph &graph, bool use_action_landmarks_, OperatorCost cost_type_);
    virtual ~LandmarkUniformSharedCostAssignment();

    virtual double cost_sharing_h_value();
};

#ifdef USE_LP
class OsiSolverInterface;
#endif

class LandmarkEfficientOptimalSharedCostAssignment : public LandmarkCostAssignment {
#ifdef USE_LP
    OsiSolverInterface *si;
#endif
public:
    explicit LandmarkEfficientOptimalSharedCostAssignment(LandmarksGraph &graph, OperatorCost cost_type_);
    virtual ~LandmarkEfficientOptimalSharedCostAssignment();

    virtual double cost_sharing_h_value();
};


#endif

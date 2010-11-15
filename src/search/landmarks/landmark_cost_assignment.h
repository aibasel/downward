#ifndef LANDMARKS_LANDMARK_COST_ASSIGNMENT_H
#define LANDMARKS_LANDMARK_COST_ASSIGNMENT_H

#include "landmarks_graph.h"

class LandmarkCostAssignment {
    const set<int> empty;
protected:
    LandmarksGraph &lm_graph;

    const set<int> &get_achievers(int lmn_status,
                                  const LandmarkNode &lmn) const;
public:
    LandmarkCostAssignment(LandmarksGraph &graph);
    virtual ~LandmarkCostAssignment();

    virtual double cost_sharing_h_value() = 0;
};

class LandmarkUniformSharedCostAssignment : public LandmarkCostAssignment {
private:
    bool use_action_landmarks;
public:
    LandmarkUniformSharedCostAssignment(LandmarksGraph &graph, bool use_action_landmarks_);
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
    LandmarkEfficientOptimalSharedCostAssignment(LandmarksGraph &graph);
    virtual ~LandmarkEfficientOptimalSharedCostAssignment();

    virtual double cost_sharing_h_value();
};


#endif

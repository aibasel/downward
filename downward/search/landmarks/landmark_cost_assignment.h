#ifndef LANDMARKS_LANDMARK_COST_ASSIGNMENT_H
#define LANDMARKS_LANDMARK_COST_ASSIGNMENT_H

#include "landmarks_graph.h"

class LandmarkCostAssignment {
protected:
    LandmarksGraph &lm_graph;
    bool exclude_ALM_effects;

    const set<int> &get_achievers(int lmn_status, LandmarkNode &lmn);
    void get_landmark_effects(const Operator &op, set<pair<int, int> > &eff);
    void get_effect_landmarks(const Operator &op, set<LandmarkNode *> &eff);

    const set<int> empty;
public:
    LandmarkCostAssignment(LandmarksGraph &graph, bool exc_ALM_eff);
    virtual ~LandmarkCostAssignment();

    virtual void assign_costs() = 0;
};

class LandmarkUniformSharedCostAssignment : public LandmarkCostAssignment {
public:
    LandmarkUniformSharedCostAssignment(LandmarksGraph &graph, bool exc_ALM_eff);
    virtual ~LandmarkUniformSharedCostAssignment();

    virtual void assign_costs();
};

#ifdef USE_LP
#pragma GCC diagnostic ignored "-Wunused-parameter"
#ifdef COIN_USE_CLP
#include "OsiClpSolverInterface.hpp"
typedef OsiClpSolverInterface OsiXxxSolverInterface;
#endif

#ifdef COIN_USE_CPX
#include "OsiCpxSolverInterface.hpp"
typedef OsiCpxSolverInterface OsiXxxSolverInterface;
#endif

#include "CoinPackedVector.hpp"
#include "CoinPackedMatrix.hpp"
#include <sys/times.h>
#endif


class LandmarkOptimalSharedCostAssignment : public LandmarkCostAssignment {
private:
#ifdef USE_LP
    OsiXxxSolverInterface * si;
#endif
    int cost_lm_i(int lm_i) {return lm_i; }
    int cost_a_lm_i(int a_i, int lm_i) {return ((a_i + 1) * lm_graph.number_of_landmarks()) + lm_i; }

public:
    LandmarkOptimalSharedCostAssignment(LandmarksGraph &graph, bool exc_ALM_eff);
    virtual ~LandmarkOptimalSharedCostAssignment();

    virtual void assign_costs();
};

/**
 * Note: This class does not take into account that landmarks
 * that have not been achieved can only be achieved by first-achievers,
 * and therefore does not give the optimal cost partitioning
 */
class LandmarkEfficientOptimalSharedCostAssignment : public LandmarkCostAssignment {
private:
#ifdef USE_LP
    OsiXxxSolverInterface * si;
#endif
public:
    LandmarkEfficientOptimalSharedCostAssignment(LandmarksGraph &graph, bool exc_ALM_eff);
    virtual ~LandmarkEfficientOptimalSharedCostAssignment();

    virtual void assign_costs();
};

#endif

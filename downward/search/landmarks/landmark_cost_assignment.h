#ifndef LANDMARK_COST_ASSIGNMENT_H_
#define LANDMARK_COST_ASSIGNMENT_H_

#include "landmarks_graph.h"

class LandmarkCostAssignment {
protected:
	LandmarksGraph &lm_graph;
	bool exclude_ALM_effects;

	const set<int>& get_achievers(int lmn_status, LandmarkNode &lmn);
	void get_landmark_effects(const Operator& op, set<pair<int, int> >& eff);

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

class LandmarkOptimalSharedCostAssignment : public LandmarkCostAssignment {
public:
	LandmarkOptimalSharedCostAssignment(LandmarksGraph &graph, bool exc_ALM_eff);
	virtual ~LandmarkOptimalSharedCostAssignment();

	virtual void assign_costs();
};

#endif /* LANDMARK_COST_ASSIGNMENT_H_ */

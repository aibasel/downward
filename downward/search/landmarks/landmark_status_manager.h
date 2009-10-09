/*
 * LandmarkStausManager.h
 *
 *  Created on: Oct 9, 2009
 *      Author: karpase
 */

#ifndef LANDMARKSTAUSMANAGER_H_
#define LANDMARKSTAUSMANAGER_H_

#include "landmarks_graph.h"
#include "../state_proxy.h"

class LandmarkStausManager {
private:
	hash_map<StateProxy, LandmarkSet> reached_lms;
	hash_map<StateProxy, ActionLandmarkSet> unused_action_lms;

	bool do_intersection;
	LandmarksGraph &lm_graph;

	bool landmark_is_leaf(const LandmarkNode& node, const LandmarkSet& reached) const;
	bool check_lost_landmark_children_needed_again(const LandmarkNode& node) const;
public:
	LandmarkStausManager(LandmarksGraph &graph);
	virtual ~LandmarkStausManager();

	void clear_reached();
	LandmarkSet& get_reached_landmarks(const State &state);
	ActionLandmarkSet& get_unused_action_landmarks(const State &state);

	bool update_lm_status(const State &state);

	void set_landmarks_for_initial_state(const State& state);
	bool update_reached_lms(const State& parent_state, const Operator &op, const State& state);

	void set_reached_landmarks(const State &state, const LandmarkSet& reached);
	void set_unused_action_landmarks(const State &state, const ActionLandmarkSet& unused);
};

#endif /* LANDMARKSTAUSMANAGER_H_ */

#ifndef LANDMARKS_LANDMARK_STATUS_MANAGER_H
#define LANDMARKS_LANDMARK_STATUS_MANAGER_H

#include "landmark_graph.h"
#include "../state_proxy.h"

class LandmarkStatusManager {
private:
    __gnu_cxx::hash_map<StateProxy, vector<bool> > reached_lms;

    bool do_intersection;
    LandmarkGraph &lm_graph;

    bool landmark_is_leaf(const LandmarkNode &node, const vector<bool> &reached) const;
    bool check_lost_landmark_children_needed_again(const LandmarkNode &node) const;
public:
    LandmarkStatusManager(LandmarkGraph &graph);
    virtual ~LandmarkStatusManager();

    void clear_reached();
    vector<bool> &get_reached_landmarks(const State &state);

    bool update_lm_status(const State &state);

    void set_landmarks_for_initial_state();
    bool update_reached_lms(const State &parent_state, const Operator &op, const State &state);
};

#endif

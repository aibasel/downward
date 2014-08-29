#ifndef LANDMARKS_LANDMARK_STATUS_MANAGER_H
#define LANDMARKS_LANDMARK_STATUS_MANAGER_H

#include "landmark_graph.h"
#include "../per_state_information.h"

class LandmarkStatusManager {
private:
    PerStateInformation<std::vector<bool> > reached_lms;

    bool do_intersection;
    LandmarkGraph &lm_graph;

    bool landmark_is_leaf(const LandmarkNode &node, const std::vector<bool> &reached) const;
    bool check_lost_landmark_children_needed_again(const LandmarkNode &node) const;
public:
    LandmarkStatusManager(LandmarkGraph &graph);
    virtual ~LandmarkStatusManager();

    std::vector<bool> &get_reached_landmarks(const GlobalState &state);

    bool update_lm_status(const GlobalState &state);

    void set_landmarks_for_initial_state();
    bool update_reached_lms(const GlobalState &parent_state, const GlobalOperator &op, const GlobalState &state);
};

#endif

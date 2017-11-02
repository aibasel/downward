#ifndef LANDMARKS_LANDMARK_STATUS_MANAGER_H
#define LANDMARKS_LANDMARK_STATUS_MANAGER_H

#include "../per_state_information.h"

namespace landmarks {
class LandmarkGraph;
class LandmarkNode;

class LandmarkStatusManager {
    PerStateInformation<std::vector<bool>> reached_lms;

    LandmarkGraph &lm_graph;
    const bool do_intersection;

    bool landmark_is_leaf(const LandmarkNode &node, const std::vector<bool> &reached) const;
    bool check_lost_landmark_children_needed_again(const LandmarkNode &node) const;
public:
    explicit LandmarkStatusManager(LandmarkGraph &graph);

    std::vector<bool> &get_reached_landmarks(const GlobalState &state);

    bool update_lm_status(const GlobalState &state);

    void set_landmarks_for_initial_state(const GlobalState &initial_state);
    bool update_reached_lms(const GlobalState &parent_state,
                            OperatorID op_id,
                            const GlobalState &state);
};
}

#endif

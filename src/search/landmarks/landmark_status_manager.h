#ifndef LANDMARKS_LANDMARK_STATUS_MANAGER_H
#define LANDMARKS_LANDMARK_STATUS_MANAGER_H

#include "../per_state_bitset.h"

namespace landmarks {
class LandmarkGraph;
class LandmarkNode;

class LandmarkStatusManager {
    PerStateBitset reached_lms;

    LandmarkGraph &lm_graph;

    bool landmark_is_leaf(const LandmarkNode &node, const BitsetView &reached) const;
    bool check_lost_landmark_children_needed_again(const LandmarkNode &node) const;
public:
    explicit LandmarkStatusManager(LandmarkGraph &graph);

    BitsetView get_reached_landmarks(const GlobalState &state);

    bool update_lm_status(const GlobalState &global_state);

    void set_landmarks_for_initial_state(const GlobalState &initial_state);
    bool update_reached_lms(const GlobalState &parent_global_state,
                            OperatorID op_id,
                            const GlobalState &global_state);
};
}

#endif

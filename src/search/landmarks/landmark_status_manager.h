#ifndef LANDMARKS_LANDMARK_STATUS_MANAGER_H
#define LANDMARKS_LANDMARK_STATUS_MANAGER_H

#include "../per_state_bitset.h"

namespace landmarks {
class LandmarkGraph;
class LandmarkNode;

enum landmark_status {lm_reached = 0, lm_not_reached = 1, lm_needed_again = 2};

class LandmarkStatusManager {
    PerStateBitset reached_lms;
    std::vector<landmark_status> lm_status;

    LandmarkGraph &lm_graph;

    bool landmark_is_leaf(const LandmarkNode &node, const BitsetView &reached) const;
    bool landmark_needed_again(int id, const GlobalState &state);
public:
    explicit LandmarkStatusManager(LandmarkGraph &graph);

    BitsetView get_reached_landmarks(const GlobalState &state);

    void update_lm_status(const GlobalState &global_state);
    bool dead_end_exists();

    void set_landmarks_for_initial_state(const GlobalState &initial_state);
    bool update_reached_lms(const GlobalState &parent_global_state,
                            OperatorID op_id,
                            const GlobalState &global_state);

    landmark_status get_landmark_status(size_t id) const;
};
}

#endif

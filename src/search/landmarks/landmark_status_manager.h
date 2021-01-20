#ifndef LANDMARKS_LANDMARK_STATUS_MANAGER_H
#define LANDMARKS_LANDMARK_STATUS_MANAGER_H

#include "../per_state_bitset.h"

namespace landmarks {
class LandmarkGraph;
class LandmarkNode;

enum landmark_status {lm_reached = 0, lm_not_reached = 1, lm_needed_again = 2};

class LandmarkStatusManager {
    PerStateBitset reached_lms;
    PerStateBitset needed_again_lms;

    LandmarkGraph &lm_graph;

    bool landmark_is_leaf(const LandmarkNode &node, const BitsetView &reached) const;
    bool check_lost_landmark_children_needed_again(
        const GlobalState &state, const LandmarkNode &node);

    int reached_cost, needed_cost, landmarks_cost;
public:
    explicit LandmarkStatusManager(LandmarkGraph &graph);

    BitsetView get_reached_landmarks(const GlobalState &state);

    bool update_lm_status(const GlobalState &global_state);

    void set_landmarks_for_initial_state(const GlobalState &initial_state);
    bool update_reached_lms(const GlobalState &parent_global_state,
                            OperatorID op_id,
                            const GlobalState &global_state);

    landmark_status get_landmark_status(
        size_t id, const GlobalState &state);

    void count_costs(const GlobalState &state);
    inline int cost_of_landmarks() const { return landmarks_cost; }
    int get_needed_cost() const { return needed_cost; }
    int get_reached_cost() const { return reached_cost; }
};
}

#endif

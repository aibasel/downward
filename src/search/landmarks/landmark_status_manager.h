#ifndef LANDMARKS_LANDMARK_STATUS_MANAGER_H
#define LANDMARKS_LANDMARK_STATUS_MANAGER_H

#include "landmark_graph.h"

#include "../per_state_bitset.h"

namespace landmarks {
class LandmarkGraph;
class LandmarkNode;

class LandmarkStatusManager {
    LandmarkGraph &lm_graph;
    const std::vector<LandmarkNode *> goal_landmarks;
    const std::vector<std::pair<LandmarkNode *, std::vector<LandmarkNode *>>> greedy_necessary_children;
    const std::vector<std::pair<LandmarkNode *, std::vector<LandmarkNode *>>> reasonable_parents;

    PerStateBitset past_landmarks;
    PerStateBitset future_landmarks;

    void progress_landmarks(
        ConstBitsetView &parent_past, ConstBitsetView &parent_future,
        const State &parent_ancestor_state, BitsetView &past,
        BitsetView &future, const State &ancestor_state);
    void progress_goals(const State &ancestor_state, BitsetView &future);
    void progress_greedy_necessary_orderings(
        const State &ancestor_state, const BitsetView &past,
        BitsetView &future);
    void progress_reasonable_orderings(
        const BitsetView &past, BitsetView &future);
public:
    LandmarkStatusManager(
        LandmarkGraph &graph,
        bool progress_goals,
        bool progress_greedy_necessary_orderings,
        bool progress_reasonable_orderings);

    BitsetView get_past_landmarks(const State &state);
    BitsetView get_future_landmarks(const State &state);
    ConstBitsetView get_past_landmarks(const State &state) const;
    ConstBitsetView get_future_landmarks(const State &state) const;

    void progress_initial_state(const State &initial_state);
    void progress(
        const State &parent_ancestor_state, OperatorID op_id,
        const State &ancestor_state);
};
}

#endif

#ifndef LANDMARKS_LANDMARK_STATUS_MANAGER_H
#define LANDMARKS_LANDMARK_STATUS_MANAGER_H

#include "landmark_graph.h"

#include "../per_state_bitset.h"

namespace landmarks {
class LandmarkGraph;
class LandmarkNode;

class LandmarkStatusManager {
    LandmarkGraph &lm_graph;
    const bool progress_goals;
    const std::vector<std::pair<LandmarkNode *, LandmarkNode *>> greedy_necessary_orderings;
    const std::vector<std::pair<LandmarkNode *, LandmarkNode *>> reasonable_orderings;

    PerStateBitset past_landmarks;
    PerStateBitset future_landmarks;

    void progress_basic_and_goals(
        const BitsetView &parent_past, const BitsetView &parent_fut,
        BitsetView &past, BitsetView &fut, const State &ancestor_state);
    void progress_greedy_necessary_orderings(
        const State &ancestor_state, const BitsetView &past, BitsetView &fut);
    void progress_reasonable_orderings(const BitsetView &past, BitsetView &fut);
public:
    LandmarkStatusManager(
        LandmarkGraph &graph,
        bool progress_goals,
        bool progress_greedy_necessary_orderings,
        bool progress_reasonable_orderings);

    BitsetView get_past_landmarks(const State &state);
    BitsetView get_future_landmarks(const State &state);

    void progress_initial_state(const State &initial_state);
    void progress(
        const State &parent_ancestor_state, OperatorID op_id,
        const State &ancestor_state);

    bool landmark_is_past(int id, const State &ancestor_state) const;
    bool landmark_is_future(int id, const State &ancestor_state) const;
};
}

#endif

#ifndef LANDMARKS_LANDMARK_STATUS_MANAGER_H
#define LANDMARKS_LANDMARK_STATUS_MANAGER_H

#include "landmark_graph.h"

#include "../per_state_bitset.h"

namespace landmarks {
class LandmarkGraph;
class LandmarkNode;

enum LandmarkStatus {PAST = 0, FUTURE = 1, PAST_AND_FUTURE = 2};

class LandmarkStatusManager {
    LandmarkGraph &lm_graph;

    PerStateBitset past_lms;
    std::vector<LandmarkStatus> lm_status;

    bool landmark_needed_again(int id, const State &state);

    void set_past_landmarks_for_initial_state(
        const State &initial_state, utils::LogProxy &log);
public:
    explicit LandmarkStatusManager(LandmarkGraph &graph);

    BitsetView get_past_landmarks(const State &state);

    void update_lm_status(const State &ancestor_state);

    void process_initial_state(
        const State &initial_state, utils::LogProxy &log);
    void progress(
        const State &parent_ancestor_state, OperatorID,
        const State &ancestor_state);

    /*
      TODO:
      The status of a landmark is actually dependent on the state. This
      is not represented in the function below. Furthermore, the status
      manager only stores the status for one particular state at a time.

      At the day of writing this comment, this works as
      *update_lm_status()* is always called before the status
      information is used (by calling *get_landmark_status()*).

      It would be a good idea to ensure that the status for the
      desired state is returned at all times, or an error is thrown
      if the desired information does not exist.
    */
    LandmarkStatus get_landmark_status(size_t id) const {
        assert(static_cast<int>(id) < lm_graph.get_num_landmarks());
        return lm_status[id];
    }
};
}

#endif

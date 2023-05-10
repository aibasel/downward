#include "landmark_status_manager.h"

#include "landmark.h"

#include "../utils/logging.h"

using namespace std;

namespace landmarks {
/*
  By default we mark all landmarks past, since we do an intersection when
  computing new landmark information.
*/
LandmarkStatusManager::LandmarkStatusManager(LandmarkGraph &graph)
    : lm_graph(graph),
      past_lms(vector<bool>(graph.get_num_landmarks(), true)),
      lm_status(graph.get_num_landmarks(), FUTURE) {
}

BitsetView LandmarkStatusManager::get_past_landmarks(const State &state) {
    return past_lms[state];
}

void LandmarkStatusManager::process_initial_state(
    const State &initial_state, utils::LogProxy &log) {
    set_past_landmarks_for_initial_state(initial_state, log);
    update_lm_status(initial_state);
}

void LandmarkStatusManager::set_past_landmarks_for_initial_state(
    const State &initial_state, utils::LogProxy &/*log*/) {
    BitsetView past = get_past_landmarks(initial_state);

    for (auto &node : lm_graph.get_nodes()) {
        if (!node->get_landmark().is_true_in_state(initial_state)) {
            past.reset(node->get_id());
        }
    }

    /*
      TODO: The old code did some logging in this function. For example, it
       printed the number of landmarks that hold in the initial state or the
       goal. I personally don't think this is generally relevant information,
       but if we want to keep that output, it should be easy to add it again.

       If we decide to get rid of the logs, we can also remove the logger from
       the function header.
    */
}

void LandmarkStatusManager::progress(
    const State &parent_ancestor_state, OperatorID,
    const State &ancestor_state) {
    if (ancestor_state == parent_ancestor_state) {
        // This can happen, e.g., in Satellite-01.
        return;
    }

    const BitsetView parent_past = get_past_landmarks(parent_ancestor_state);
    BitsetView past = get_past_landmarks(ancestor_state);

    int num_landmarks = lm_graph.get_num_landmarks();
    assert(past.size() == num_landmarks);
    assert(parent_past.size() == num_landmarks);

    for (int id = 0; id < num_landmarks; ++id) {
        /*
          TODO: Computing whether a landmark is true in a state is expensive.
           It can happen that we compute this multiple times for the same
           state. If we observe a slow-down in the experiments, we could look
           into this as an opportunity to speed things up.
        */
        if (!parent_past.test(id) && past.test(id)
            && !lm_graph.get_node(id)->get_landmark().is_true_in_state(
                ancestor_state)) {
            past.reset(id);
        }
    }
}

void LandmarkStatusManager::update_lm_status(const State &ancestor_state) {
    const BitsetView past = get_past_landmarks(ancestor_state);

    const int num_landmarks = lm_graph.get_num_landmarks();
    /* This first loop is necessary as setup for the needed-again
       check in the second loop. */
    for (int id = 0; id < num_landmarks; ++id) {
        lm_status[id] = past.test(id) ? PAST : FUTURE;
    }
    for (int id = 0; id < num_landmarks; ++id) {
        if (lm_status[id] == PAST
            && landmark_needed_again(id, ancestor_state)) {
            lm_status[id] = PAST_AND_FUTURE;
        }
    }
}

bool LandmarkStatusManager::landmark_needed_again(
    int id, const State &state) {
    LandmarkNode *node = lm_graph.get_node(id);
    const Landmark &landmark = node->get_landmark();
    if (landmark.is_true_in_state(state)) {
        return false;
    } else if (landmark.is_true_in_goal) {
        return true;
    } else {
        /*
          For all A ->_gn B, if B is not past and A currently not
          true, since A is a necessary precondition for actions
          achieving B for the first time, A must become true again.
        */
        for (const auto &child : node->children) {
            if (child.second >= EdgeType::GREEDY_NECESSARY
                && lm_status[child.first->get_id()] == FUTURE) {
                return true;
            }
        }
        return false;
    }
}
}

#include "landmark_status_manager.h"

#include "landmark_graph.h"

#include "../utils/logging.h"

using namespace std;

namespace landmarks {
/*
  By default we mark all landmarks as reached, since we do an intersection when
  computing new landmark information.
*/
LandmarkStatusManager::LandmarkStatusManager(LandmarkGraph &graph)
    : reached_lms(vector<bool>(graph.get_num_landmarks(), true)),
      lm_status(graph.get_num_landmarks(), lm_not_reached),
      lm_graph(graph) {
}

landmark_status LandmarkStatusManager::get_landmark_status(
    size_t id) const {
    assert(static_cast<int>(id) < lm_graph.get_num_landmarks());
    return lm_status[id];
}

BitsetView LandmarkStatusManager::get_reached_landmarks(const State &state) {
    return reached_lms[state];
}

void LandmarkStatusManager::set_landmarks_for_initial_state(
    const State &initial_state) {
    BitsetView reached = get_reached_landmarks(initial_state);
    // This is necessary since the default is "true for all" (see comment above).
    reached.reset();

    int inserted = 0;
    int num_goal_lms = 0;
    for (auto &node_p : lm_graph.get_nodes()) {
        if (node_p->is_true_in_goal) {
            ++num_goal_lms;
        }

        if (!node_p->parents.empty()) {
            continue;
        }
        if (node_p->conjunctive) {
            bool lm_true = true;
            for (const FactPair &fact : node_p->facts) {
                if (initial_state[fact.var].get_value() != fact.value) {
                    lm_true = false;
                    break;
                }
            }
            if (lm_true) {
                reached.set(node_p->get_id());
                ++inserted;
            }
        } else {
            for (const FactPair &fact : node_p->facts) {
                if (initial_state[fact.var].get_value() == fact.value) {
                    reached.set(node_p->get_id());
                    ++inserted;
                    break;
                }
            }
        }
    }
    utils::g_log << inserted << " initial landmarks, "
                 << num_goal_lms << " goal landmarks" << endl;
}

bool LandmarkStatusManager::update_reached_lms(
    const State &parent_ancestor_state,OperatorID,
    const State &ancestor_state) {
    if (ancestor_state == parent_ancestor_state) {
        // This can happen, e.g., in Satellite-01.
        return false;
    }

    const BitsetView parent_reached = get_reached_landmarks(
        parent_ancestor_state);
    BitsetView reached = get_reached_landmarks(ancestor_state);

    int num_landmarks = lm_graph.get_num_landmarks();
    assert(reached.size() == num_landmarks);
    assert(parent_reached.size() == num_landmarks);

    /*
       Set all landmarks not reached by this parent as "not reached".
       Over multiple paths, this has the effect of computing the intersection
       of "reached" for the parents. It is important here that upon first visit,
       all elements in "reached" are true because true is the neutral element
       of intersection.

       In the case where the landmark we are setting to false here is actually
       achieved right now, it is set to "true" again below.
    */
    reached.intersect(parent_reached);


    // Mark landmarks reached right now as "reached" (if they are "leaves").
    for (int id = 0; id < num_landmarks; ++id) {
        if (!reached.test(id)) {
            LandmarkNode *node = lm_graph.get_landmark(id);
            if (node->is_true_in_state(ancestor_state)) {
                reached.set(id);
            }
        }
    }

    return true;
}

void LandmarkStatusManager::update_lm_status(const State &ancestor_state) {
    const BitsetView reached = get_reached_landmarks(ancestor_state);

    /* This first loop is necessary as setup for the *needed again*
       check in the second loop. */
    for (int id = 0; id < lm_graph.get_num_landmarks(); ++id) {
        lm_status[id] = reached.test(id) ? lm_reached : lm_not_reached;
    }
    for (int id = 0; id < lm_graph.get_num_landmarks(); ++id) {
        if (lm_status[id] == lm_reached
            && landmark_needed_again(id, ancestor_state)) {
            lm_status[id] = lm_needed_again;
        }
    }
}

bool LandmarkStatusManager::dead_end_exists() {
    for (auto &node : lm_graph.get_nodes()) {
        int id = node->get_id();

        /*
          This dead-end detection works for the following case:
          X is a goal, it is true in the initial state, and has no achievers.
          Some action A has X as a delete effect. Then using this,
          we can detect that applying A leads to a dead-end.

          Note: this only tests for reachability of the landmark from the initial state.
          A (possibly) more effective option would be to test reachability of the landmark
          from the current state.
        */

        if (!node->is_derived) {
            if ((lm_status[id] == lm_not_reached) &&
                node->first_achievers.empty()) {
                return true;
            }
            if ((lm_status[id] == lm_needed_again) &&
                node->possible_achievers.empty()) {
                return true;
            }
        }
    }
    return false;
}

bool LandmarkStatusManager::landmark_needed_again(
    int id, const State &state) {
    LandmarkNode *node = lm_graph.get_landmark(id);
    if (node->is_true_in_state(state)) {
        return has_unreached_parent(node);
    } else if (node->is_true_in_goal) {
        return true;
    } else {
        /*
          For all A ->_gn B, if B is not reached and A currently not
          true, since A is a necessary precondition for actions
          achieving B for the first time, it must become true again.
        */
        for (const auto &child : node->children) {
            if (child.second >= EdgeType::GREEDY_NECESSARY
                && lm_status[child.first->get_id()] == lm_not_reached) {
                return true;
            }
        }
        return has_unreached_parent(node);
    }
}

bool LandmarkStatusManager::has_unreached_parent(
    const LandmarkNode *node) const {
    for (const auto &parent : node->parents) {
        if (lm_status[parent.first->get_id()] == lm_not_reached) {
            // This cannot occur for natural orderings by definition.
            assert(parent.second < EdgeType::NATURAL);
            return true;
        }
    }
    return false;
}
}

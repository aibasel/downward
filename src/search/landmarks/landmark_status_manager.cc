#include "landmark_status_manager.h"

#include "landmark.h"

#include "../utils/logging.h"

using namespace std;

namespace landmarks {
/*
  By default we mark all landmarks as reached, since we do an intersection when
  computing new landmark information.
*/
LandmarkStatusManager::LandmarkStatusManager(LandmarkGraph &graph)
    : lm_graph(graph),
      task_is_unsolvable(false),
      reached_lms(vector<bool>(graph.get_num_landmarks(), true)),
      lm_status(graph.get_num_landmarks(), lm_not_reached) {
    compute_unachievable_landmark_ids();
}

BitsetView LandmarkStatusManager::get_reached_landmarks(const State &state) {
    return reached_lms[state];
}

void LandmarkStatusManager::process_initial_state(
    const State &initial_state, utils::LogProxy &log) {
    set_reached_landmarks_for_initial_state(initial_state, log);
    update_lm_status(initial_state);
    task_is_unsolvable = is_initial_state_dead_end();
}

void LandmarkStatusManager::set_reached_landmarks_for_initial_state(
    const State &initial_state, utils::LogProxy &log) {
    BitsetView reached = get_reached_landmarks(initial_state);
    // This is necessary since the default is "true for all" (see comment above).
    reached.reset();

    int inserted = 0;
    int num_goal_lms = 0;
    for (auto &lm_node : lm_graph.get_nodes()) {
        const Landmark &landmark = lm_node->get_landmark();
        if (landmark.is_true_in_goal) {
            ++num_goal_lms;
        }

        if (!lm_node->parents.empty()) {
            continue;
        }
        if (landmark.conjunctive) {
            bool lm_true = true;
            for (const FactPair &fact : landmark.facts) {
                if (initial_state[fact.var].get_value() != fact.value) {
                    lm_true = false;
                    break;
                }
            }
            if (lm_true) {
                reached.set(lm_node->get_id());
                ++inserted;
            }
        } else {
            for (const FactPair &fact : landmark.facts) {
                if (initial_state[fact.var].get_value() == fact.value) {
                    reached.set(lm_node->get_id());
                    ++inserted;
                    break;
                }
            }
        }
    }
    if (log.is_at_least_normal()) {
        log << inserted << " initial landmarks, "
            << num_goal_lms << " goal landmarks" << endl;
    }
}

bool LandmarkStatusManager::process_state_transition(
    const State &parent_ancestor_state, OperatorID,
    const State &ancestor_state) {
    if (ancestor_state == parent_ancestor_state) {
        // This can happen, e.g., in Satellite-01.
        return false;
    }

    const BitsetView parent_reached = get_reached_landmarks(parent_ancestor_state);
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
            LandmarkNode *node = lm_graph.get_node(id);
            if (node->get_landmark().is_true_in_state(ancestor_state)) {
                if (landmark_is_leaf(*node, reached)) {
                    reached.set(id);
                }
            }
        }
    }

    return true;
}

void LandmarkStatusManager::update_lm_status(const State &ancestor_state) {
    const BitsetView reached = get_reached_landmarks(ancestor_state);

    const int num_landmarks = lm_graph.get_num_landmarks();
    /* This first loop is necessary as setup for the needed-again
       check in the second loop. */
    for (int id = 0; id < num_landmarks; ++id) {
        lm_status[id] = reached.test(id) ? lm_reached : lm_not_reached;
    }
    for (int id = 0; id < num_landmarks; ++id) {
        if (lm_status[id] == lm_reached
            && landmark_needed_again(id, ancestor_state)) {
            lm_status[id] = lm_needed_again;
        }
    }
}

bool LandmarkStatusManager::is_initial_state_dead_end() const {
    /*
      Note: Landmark statuses must be set for the initial state
      prior to calling this function.
    */
    for (auto &lm_node : lm_graph.get_nodes()) {
        const Landmark &landmark = lm_node->get_landmark();
        int id = lm_node->get_id();
        /*
          TODO: We skip derived landmarks because they can have
          "hidden achievers". In the future, deal with this in a more
          principled way.
        */
        if (!landmark.is_derived) {
            if ((lm_status[id] == lm_not_reached) &&
                landmark.first_achievers.empty()) {
                return true;
            }
        }
    }
    return false;
}

void LandmarkStatusManager::compute_unachievable_landmark_ids() {
    for (auto &node : lm_graph.get_nodes()) {
        int id = node->get_id();
        const Landmark &landmark = node->get_landmark();
        /*
          TODO: We skip derived landmarks because they can have
          "hidden achievers". In the future, deal with this in a more
          principled way.
        */
        if (!landmark.is_derived && landmark.possible_achievers.empty()) {
            unachievable_landmark_ids.push_back(id);
        }
    }
}

bool LandmarkStatusManager::dead_end_exists() const {
    if (task_is_unsolvable) {
        return true;
    }
    for (int id : unachievable_landmark_ids) {
        /*
          For efficiency, we only check needed-again landmarks,
          not unreached landmarks. We assume that unreached landmarks
          are captured by *task_is_unsolvable*.
        */
        if (lm_status[id] == lm_needed_again) {
            return true;
        }
    }
    return false;
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
        return false;
    }
}

bool LandmarkStatusManager::landmark_is_leaf(const LandmarkNode &node,
                                             const BitsetView &reached) const {
    //Note: this is the same as !check_node_orders_disobeyed
    for (const auto &parent : node.parents) {
        LandmarkNode *parent_node = parent.first;
        // Note: no condition on edge type here
        if (!reached.test(parent_node->get_id())) {
            return false;
        }
    }
    return true;
}
}

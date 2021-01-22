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
    : reached_lms(vector<bool>(graph.number_of_landmarks(), true)),
      lm_status(graph.number_of_landmarks(), lm_not_reached),
      lm_graph(graph) {
}

landmark_status LandmarkStatusManager::get_landmark_status(
    size_t id, const GlobalState &state) {

    assert(0 <= id && id < lm_graph.number_of_landmarks());
    return lm_status[id];
}

BitsetView
LandmarkStatusManager::get_reached_landmarks(const GlobalState &state) {
    return reached_lms[state];
}

void LandmarkStatusManager::set_landmarks_for_initial_state(
    const GlobalState &initial_state) {
    BitsetView reached = get_reached_landmarks(initial_state);
    // This is necessary since the default is "true for all" (see comment above).
    reached.reset();

    int inserted = 0;
    int num_goal_lms = 0;
    for (auto &node_p : lm_graph.get_nodes()) {
        if (node_p->in_goal) {
            ++num_goal_lms;
        }

        if (!node_p->parents.empty()) {
            continue;
        }
        if (node_p->conjunctive) {
            bool lm_true = true;
            for (const FactPair &fact : node_p->facts) {
                if (initial_state[fact.var] != fact.value) {
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
                if (initial_state[fact.var] == fact.value) {
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
    const GlobalState &parent_global_state,
    OperatorID,
    const GlobalState &global_state) {
    if (global_state.get_id() == parent_global_state.get_id()) {
        // This can happen, e.g., in Satellite-01.
        return false;
    }

    const BitsetView parent_reached = get_reached_landmarks(
        parent_global_state);
    BitsetView reached = get_reached_landmarks(global_state);

    int num_landmarks = lm_graph.number_of_landmarks();
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
            LandmarkNode* node = lm_graph.get_lm_for_index(id);
            if (node->is_true_in_state(global_state)) {
                if (landmark_is_leaf(*node, reached)) {
                    reached.set(id);
                }
            }
        }
    }

    return true;
}

void LandmarkStatusManager::update_lm_status(const GlobalState &global_state) {
    const BitsetView reached = get_reached_landmarks(global_state);

    const LandmarkGraph::Nodes &nodes = lm_graph.get_nodes();
    for (auto &node : nodes) {
        if (reached.test(node->get_id())) {
            if (landmark_needed_again(node->get_id(), global_state)) {
                lm_status[node->get_id()] = lm_needed_again;
            } else {
                lm_status[node->get_id()] = lm_reached;
            }
        } else {
            lm_status[node->get_id()] = lm_not_reached;
        }
    }
}

bool LandmarkStatusManager::dead_end_exists(const GlobalState &global_state) {
    for (auto &node : lm_graph.get_nodes()) {
        int id = node->get_id();

        // This dead-end detection works for the following case:
        // X is a goal, it is true in the initial state, and has no achievers.
        // Some action A has X as a delete effect. Then using this,
        // we can detect that applying A leads to a dead-end.
        //
        // Note: this only tests for reachability of the landmark from the initial state.
        // A (possibly) more effective option would be to test reachability of the landmark
        // from the current state.

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

bool LandmarkStatusManager::check_lost_landmark_children_needed_again(
    const GlobalState &state, const LandmarkNode &node) {

    for (const auto &child : node.children) {
        LandmarkNode* child_node = child.first;
        if (child.second >= EdgeType::greedy_necessary
            && get_landmark_status(child_node->get_id(), state)
                == lm_not_reached)
            return true;
    }
    return false;
}

bool LandmarkStatusManager::landmark_is_leaf(const LandmarkNode &node,
                                             const BitsetView &reached) const {
    //Note: this is the same as !check_node_orders_disobeyed
    for (const auto &parent : node.parents) {
        LandmarkNode* parent_node = parent.first;
        // Note: no condition on edge type here
        if (!reached.test(parent_node->get_id())) {
            return false;
        }
    }
    return true;
}

bool
LandmarkStatusManager::landmark_needed_again(
    int id, const GlobalState &state) {

    LandmarkNode* node = lm_graph.get_lm_for_index(id);
    if (node->is_true_in_state(state)) {
        return false;
    } else if (node->is_goal()) {
        return true;
    } else {
        return check_lost_landmark_children_needed_again(state, *node);
    }
}
}

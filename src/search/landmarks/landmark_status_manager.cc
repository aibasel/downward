#include "landmark_status_manager.h"

#include "landmark_graph.h"

using namespace std;

namespace landmarks {
/*
  By default we mark all landmarks as reached, since we do an intersection when
  computing new landmark information.
*/
LandmarkStatusManager::LandmarkStatusManager(LandmarkGraph &graph)
    : reached_lms(vector<bool>(graph.number_of_landmarks(), true)),
      lm_graph(graph) {
}

BitsetView LandmarkStatusManager::get_reached_landmarks(const GlobalState &state) {
    return reached_lms[state];
}

void LandmarkStatusManager::set_landmarks_for_initial_state(
    const GlobalState &initial_state) {
    BitsetView reached = get_reached_landmarks(initial_state);
    // This is necessary since the default is "true for all" (see comment above).
    reached.reset();

    int inserted = 0;
    int num_goal_lms = 0;
    const set<LandmarkNode *> &nodes = lm_graph.get_nodes();
    for (set<LandmarkNode *>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        LandmarkNode *node_p = *it;
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
    cout << inserted << " initial landmarks, "
         << num_goal_lms << " goal landmarks" << endl;
}


bool LandmarkStatusManager::update_reached_lms(const GlobalState &parent_global_state,
                                               OperatorID,
                                               const GlobalState &global_state) {
    if (global_state.get_id() == parent_global_state.get_id()) {
        // This can happen, e.g., in Satellite-01.
        return false;
    }

    const BitsetView parent_reached = get_reached_landmarks(parent_global_state);
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
            LandmarkNode *node = lm_graph.get_lm_for_index(id);
            if (node->is_true_in_state(global_state)) {
                if (landmark_is_leaf(*node, reached)) {
                    reached.set(id);
                }
            }
        }
    }

    return true;
}

bool LandmarkStatusManager::update_lm_status(const GlobalState &global_state) {
    const BitsetView reached = get_reached_landmarks(global_state);

    const set<LandmarkNode *> &nodes = lm_graph.get_nodes();
    // initialize all nodes to not reached and not effect of unused ALM
    for (LandmarkNode *node : nodes) {
        node->status = lm_not_reached;
        if (reached.test(node->get_id())) {
            node->status = lm_reached;
        }
    }

    bool dead_end_found = false;

    // mark reached and find needed again landmarks
    for (LandmarkNode *node : nodes) {
        if (node->status == lm_reached) {
            if (!node->is_true_in_state(global_state)) {
                if (node->is_goal()) {
                    node->status = lm_needed_again;
                } else {
                    if (check_lost_landmark_children_needed_again(*node)) {
                        node->status = lm_needed_again;
                    }
                }
            }
        }

        // This dead-end detection works for the following case:
        // X is a goal, it is true in the initial state, and has no achievers.
        // Some action A has X as a delete effect. Then using this,
        // we can detect that applying A leads to a dead-end.
        //
        // Note: this only tests for reachability of the landmark from the initial state.
        // A (possibly) more effective option would be to test reachability of the landmark
        // from the current state.

        if (!node->is_derived) {
            if ((node->status == lm_not_reached) &&
                node->first_achievers.empty()) {
                dead_end_found = true;
            }
            if ((node->status == lm_needed_again) &&
                node->possible_achievers.empty()) {
                dead_end_found = true;
            }
        }
    }

    return dead_end_found;
}


bool LandmarkStatusManager::check_lost_landmark_children_needed_again(const LandmarkNode &node) const {
    for (const auto &child : node.children) {
        LandmarkNode *child_node = child.first;
        if (child.second >= EdgeType::greedy_necessary && child_node->status == lm_not_reached)
            return true;
    }
    return false;
}

bool LandmarkStatusManager::landmark_is_leaf(const LandmarkNode &node, const BitsetView &reached) const {
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

#include "landmark_status_manager.h"

using namespace __gnu_cxx;

LandmarkStatusManager::LandmarkStatusManager(LandmarkGraph &graph)
    : lm_graph(graph) {
    do_intersection = true;
}

LandmarkStatusManager::~LandmarkStatusManager() {
}


void LandmarkStatusManager::clear_reached() {
    reached_lms.clear();
}

vector<bool> &LandmarkStatusManager::get_reached_landmarks(const State &state) {
    StateProxy proxy = StateProxy(&state);

    //assert(reached_lms.find(proxy) != reached_lms.end());
    return reached_lms[proxy];
}


void LandmarkStatusManager::set_landmarks_for_initial_state() {
    vector<bool> &reached = get_reached_landmarks(*g_initial_state);
    reached.resize(lm_graph.number_of_landmarks());
    //cout << "NUMBER OF LANDMARKS: " << lm_graph.number_of_landmarks() << endl;

    int inserted = 0;
    int num_goal_lms = 0;
    // opt: initial_state_landmarks.resize(lm_graph->number_of_landmarks());
    const set<LandmarkNode *> &nodes = lm_graph.get_nodes();
    for (set<LandmarkNode *>::iterator it = nodes.begin(); it != nodes.end(); it++) {
        LandmarkNode *node_p = *it;
        if (node_p->in_goal) {
            num_goal_lms++;
        }

        if (node_p->parents.size() > 0) {
            continue;
        }
        if (node_p->conjunctive) {
            bool lm_true = true;
            for (int i = 0; i < node_p->vals.size(); i++) {
                if ((*g_initial_state)[node_p->vars[i]] != node_p->vals[i]) {
                    lm_true = false;
                    break;
                }
            }
            if (lm_true) {
                reached[node_p->get_id()] = true;
                inserted++;
            }
        } else {
            for (int i = 0; i < node_p->vals.size(); i++) {
                if ((*g_initial_state)[node_p->vars[i]] == node_p->vals[i]) {
                    reached[node_p->get_id()] = true;
                    inserted++;
                    break;
                }
            }
        }
    }
    cout << inserted << " initial landmarks, "
         << num_goal_lms << " goal landmarks" << endl;
}


bool LandmarkStatusManager::update_reached_lms(
    const State &parent_state, const Operator &, const State &state) {
    vector<bool> &parent_reached = get_reached_landmarks(parent_state);
    vector<bool> &reached = get_reached_landmarks(state);


    if (&parent_reached == &reached) {
        assert(state == parent_state);
        // This can happen, e.g., in Satellite-01.
        return false;
    }

    bool intersect = (do_intersection && !reached.empty());
    vector<bool> old_reached;
    if (intersect) {
        // Save old reached landmarks for this state.
        // Swapping is faster than old_reached = reached, and we assign
        // a new value to reached next anyway.
        old_reached.swap(reached);
    }

    reached = parent_reached;



    int num_landmarks = lm_graph.number_of_landmarks();
    assert(reached.size() == num_landmarks);
    assert(parent_reached.size() == num_landmarks);
    assert(!intersect || old_reached.size() == num_landmarks);

    for (int id = 0; id < num_landmarks; ++id) {
        if (intersect && !old_reached[id]) {
            reached[id] = false;
        } else if (!reached[id]) {
            LandmarkNode *node = lm_graph.get_lm_for_index(id);
            if (node->is_true_in_state(state)) {
                // cout << "New LM reached: id " << id << " ";
                // lm_graph.dump_node(node);
                if (landmark_is_leaf(*node, reached)) {
                    //      cout << "inserting new LM into reached. (2)" << endl;
                    reached[id] = true;
                }
                /*
                   else {
                   cout << "non-leaf, not inserting." << std::endl;
                   }
                 */
            }
        }
    }


    return true;
}

bool LandmarkStatusManager::update_lm_status(const State &state) {
    vector<bool> &reached = get_reached_landmarks(state);

    const set<LandmarkNode *> &nodes = lm_graph.get_nodes();
    // initialize all nodes to not reached and not effect of unused ALM
    set<LandmarkNode *>::iterator lit;
    for (lit = nodes.begin(); lit != nodes.end(); lit++) {
        LandmarkNode &node = **lit;
        node.status = lm_not_reached;
        if (reached[node.get_id()]) {
            node.status = lm_reached;
        }
    }

    bool dead_end_found = false;

    // mark reached and find needed again landmarks
    for (lit = nodes.begin(); lit != nodes.end(); lit++) {
        LandmarkNode &node = **lit;
        if (node.status == lm_reached) {
            if (!node.is_true_in_state(state)) {
                if (node.is_goal()) {
                    node.status = lm_needed_again;
                } else {
                    if (check_lost_landmark_children_needed_again(node)) {
                        node.status = lm_needed_again;
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

        if (!node.is_derived) {
            if ((node.status == lm_not_reached) &&
                (node.first_achievers.size() == 0)) {
                dead_end_found = true;
            }
            if ((node.status == lm_needed_again) &&
                (node.possible_achievers.size() == 0)) {
                dead_end_found = true;
            }
        }
    }

    return dead_end_found;
}


bool LandmarkStatusManager::check_lost_landmark_children_needed_again(const LandmarkNode &node) const {
    const hash_map<LandmarkNode *, edge_type, hash_pointer > &children =
        node.children;

    for (hash_map<LandmarkNode *, edge_type, hash_pointer >::const_iterator child_it =
             children.begin(); child_it != children.end(); child_it++) {
        LandmarkNode *child_p = child_it->first;
        if (child_it->second >= greedy_necessary &&
            child_p->status == lm_not_reached)
            return true;
    }
    return false;
}

bool LandmarkStatusManager::landmark_is_leaf(const LandmarkNode &node,
                                             const vector<bool> &reached) const {
//Note: this is the same as !check_node_orders_disobeyed
    const hash_map<LandmarkNode *, edge_type, hash_pointer > &parents =
        node.parents;
    /*
      cout << "in is_leaf, reached is ----- " << endl;
      hash_set<const LandmarkNode*, hash_pointer>::const_iterator it;
      for(it = reached.begin(); it != reached.end(); ++it) {
      cout << *it << " ";
      lgraph.dump_node(*it);
      }
      cout << "---------" << endl;
    */
    for (hash_map<LandmarkNode *, edge_type, hash_pointer >::const_iterator parent_it =
             parents.begin(); parent_it != parents.end(); parent_it++) {
        LandmarkNode *parent_p = parent_it->first;

        if (true) // Note: no condition on edge type here
            if (!reached[parent_p->get_id()]) {
                //cout << "parent is not in reached: ";
                //cout << parent_p << " ";
                //lm_graph.dump_node(parent_p);
                return false;
            }

    }
    //cout << "all parents are in reached" << endl;
    return true;
}

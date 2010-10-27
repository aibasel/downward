#include "landmark_status_manager.h"

LandmarkStatusManager::LandmarkStatusManager(LandmarksGraph &graph)
    : lm_graph(graph) {
    do_intersection = true;
}

LandmarkStatusManager::~LandmarkStatusManager() {
}


void LandmarkStatusManager::clear_reached() {
    reached_lms.clear();
    unused_action_lms.clear();
}

LandmarkSet &LandmarkStatusManager::get_reached_landmarks(const State &state) {
    StateProxy proxy = StateProxy(&state);

    //assert(reached_lms.find(proxy) != reached_lms.end());
    return reached_lms[proxy];
}

ActionLandmarkSet &LandmarkStatusManager::get_unused_action_landmarks(const State &state) {
    StateProxy proxy = StateProxy(&state);

    //assert(unused_action_lms.find(proxy) != unused_action_lms.end());
    return unused_action_lms[proxy];
}

void LandmarkStatusManager::set_landmarks_for_initial_state(const State &state) {
    LandmarkSet &reached = get_reached_landmarks(state);
    ActionLandmarkSet &unused_alm = get_unused_action_landmarks(state);

    int inserted = 0;
    // opt: initial_state_landmarks.resize(lm_graph->number_of_landmarks());
    for (int i = 0; i < g_variable_domain.size(); i++) {
        const pair<int, int> a = make_pair(i, state[i]);
        if (lm_graph.simple_landmark_exists(a)) {
            LandmarkNode &node = lm_graph.get_simple_lm_node(a);
            if (node.parents.size() == 0) {
                reached.insert(&node);
                inserted++;
            }
        } else {
            set<pair<int, int> > a_set;
            a_set.insert(a);
            if (lm_graph.disj_landmark_exists(a_set)) {
                LandmarkNode &node = lm_graph.get_disj_lm_node(a);
                if (node.parents.size() == 0) {
                    reached.insert(&node);
                    inserted++;
                }
            }
        }
    }
    cout << inserted << " initial landmarks, "
         << g_goal.size() << " goal landmarks" << endl;

    unused_alm.insert(
        lm_graph.get_action_landmarks().begin(),
        lm_graph.get_action_landmarks().end());
}


bool LandmarkStatusManager::update_reached_lms(const State &parent_state, const Operator &op,
                                               const State &state) {
    //int reached_size = 0;
    bool intersect_used = false;

    // collect data from old path(s)
    bool intersect = false;

    LandmarkSet old_reached;

    StateProxy proxy = StateProxy(&parent_state);
    assert(reached_lms.find(proxy) != reached_lms.end());

    LandmarkSet &parent_reached = get_reached_landmarks(parent_state);
    LandmarkSet &reached = get_reached_landmarks(state);

    // save old reached landmarks for this state
    if (do_intersection) {
        old_reached.insert(reached.begin(), reached.end());
        intersect = (old_reached.size() > 0);
    }

    // todo: this can be optimized - clear only the landmarks that should be erased

    // clear previous information about reached landmarks
    reached.clear();
    // insert landmarks that were reached in the parent state
    reached.insert(parent_reached.begin(), parent_reached.end());

    const set<LandmarkNode *> &nodes = lm_graph.get_nodes();

    for (int j = 0; j < op.get_pre_post().size(); j++) {
        const PrePost &pre_post = op.get_pre_post()[j];
        //if(pre_post.does_fire(*this)) { //Silvia: leaving the old line in for now, for experiments
        // (we want to be able to put this line back in, so that previous behavior is exhibited)
        //
        // Test whether this effect got applied (it may have been conditional)
        if (state[pre_post.var] == pre_post.post) {
            LandmarkNode *node_p = lm_graph.landmark_reached(make_pair(pre_post.var, pre_post.post));
            if (node_p != NULL) {
                if (reached.find(node_p) == reached.end()) {
                    LandmarkNode &node = *node_p;
                    if (landmark_is_leaf(node, reached)) {
                        reached.insert(node_p);
                    }
                }
            }
        }
    }

    // update landmarks that were reached by axioms
    set<LandmarkNode *>::const_iterator it = nodes.begin();
    for (; it != nodes.end(); ++it) {
        LandmarkNode *node = *it;
        for (int i = 0; i < node->vars.size(); i++) {
            if (state[node->vars[i]] == node->vals[i]) {
                if (reached.find(node) == reached.end()) {
                    //cout << "New LM reached by axioms: "; g_lgraph->dump_node(node);
                    if (landmark_is_leaf(*node, reached)) {
                        //cout << "inserting new LM into reached. (2)" << endl;
                        reached.insert(node);
                    }
                    //else
                    //cout << "not inserting into reached, has parents" << endl;
                }
            }
        }
    }

    // intersect the new reached landmarks (in reached) with the old reached landmarks (int old_reached)
    if (intersect) {
        LandmarkSet::iterator reached_it;
        vector<LandmarkNode *> to_erase;
        // find the landmarks that should be deleted
        for (reached_it = reached.begin(); reached_it != reached.end(); reached_it++) {
            LandmarkNode *node_p = *reached_it;
            if (old_reached.find(node_p) == old_reached.end()) {
                // landmark not found in old_reached, erase it
                to_erase.push_back(node_p);
            }
        }

        for (int i = 0; i < to_erase.size(); i++) {
            reached.erase(to_erase[i]);
            intersect_used = true;
        }
    }

    // handle action landmarks
    ActionLandmarkSet &parent_unused_alm = get_unused_action_landmarks(parent_state);
    ActionLandmarkSet &unused_alm = get_unused_action_landmarks(state);

    // this is actually union here, so if we don't do union, we start from scratch
    if (!do_intersection) {
        unused_alm.clear();
        // todo: we should check if the union added something here. For now just return true
    }

    // insert all the unused action landmarks from the parent
    unused_alm.insert(parent_unused_alm.begin(), parent_unused_alm.end());

    // and erase the last action
    if (unused_alm.find(&op) != unused_alm.end()) {
        unused_alm.erase(&op);
    }

    if (!intersect)
        assert(reached.size() >= parent_reached.size());
    return true;
}

bool LandmarkStatusManager::update_lm_status(const State &state) {
    StateProxy proxy = StateProxy(&state);

    LandmarkSet &reached = get_reached_landmarks(state);
    ActionLandmarkSet &unused_alm = get_unused_action_landmarks(state);

    const set<LandmarkNode *> &nodes = lm_graph.get_nodes();
    // initialize all nodes to not reached and not effect of unused ALM
    set<LandmarkNode *>::iterator lit;
    for (lit = nodes.begin(); lit != nodes.end(); lit++) {
        LandmarkNode &node = **lit;
        node.status = lm_not_reached;
        node.effect_of_ununsed_alm = false;
        if (reached.find(&node) != reached.end()) {
            node.status = lm_reached;
        }
    }

    int unused_alm_cost = 0;
    // go over all unused action landmarks
    for (set<const Operator *>::const_iterator action_lm_it = unused_alm.begin();
         action_lm_it != unused_alm.end(); action_lm_it++) {
        const Operator *action_lm = *action_lm_it;
        // mark all landmark effects as effects of unused action landmark
        for (int i = 0; i < action_lm->get_pre_post().size(); i++) {
            const PrePost &pre_post = action_lm->get_pre_post()[i];
            LandmarkNode *node_p = lm_graph.landmark_reached(make_pair(pre_post.var, pre_post.post));
            if (node_p != NULL) {
                node_p->effect_of_ununsed_alm = true;
            }
        }
        unused_alm_cost += action_lm->get_cost();
    }
    lm_graph.set_unused_action_landmark_cost(unused_alm_cost);

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

        if (!node.is_derived) {
            if ((node.status == lm_not_reached) && (node.first_achievers.size() == 0)) {
                dead_end_found = true;
            }
            if ((node.status == lm_needed_again) && (node.possible_achievers.size() == 0)) {
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
        if (child_it->second == gn) // Note: condition on edge type here!
            //if(reached.find(child_p) == reached.end()){
            if (child_p->status == lm_not_reached) {
                return true;
            }

    }
    return false;
}

bool LandmarkStatusManager::landmark_is_leaf(const LandmarkNode &node,
                                             const LandmarkSet &reached) const {
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

            if (reached.find(parent_p) == reached.end()) {
                //cout << "parent is not in reached: ";
                //cout << parent_p << " ";
                //lgraph.dump_node(parent_p);
                return false;
            }

    }
    //cout << "all parents are in reached" << endl;
    return true;
}

void LandmarkStatusManager::set_reached_landmarks(const State &state, const LandmarkSet &reached) {
    StateProxy proxy = StateProxy(&state);

    // new state
    if (reached_lms.find(proxy) == reached_lms.end()) {
        reached_lms[proxy].resize(reached.size());
    }

    reached_lms[proxy].insert(reached.begin(), reached.end());
}

void LandmarkStatusManager::set_unused_action_landmarks(const State &state,
                                                        const ActionLandmarkSet &unused) {
    StateProxy proxy = StateProxy(&state);

    // new state
    if (unused_action_lms.find(proxy) == unused_action_lms.end()) {
        unused_action_lms[proxy].clear();
    }

    unused_action_lms[proxy].insert(unused.begin(), unused.end());
}

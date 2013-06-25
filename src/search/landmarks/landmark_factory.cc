#include "landmark_factory.h"
#include "../exact_timer.h"
#include "util.h"

#include <limits>
#include <fstream>

using namespace __gnu_cxx;

LandmarkFactory::LandmarkFactory(const Options &opts)
    : lm_graph(new LandmarkGraph(opts)) {
}

LandmarkGraph *LandmarkFactory::compute_lm_graph() {
    ExactTimer lm_generation_timer;
    generate_landmarks();

    // the following replaces the old "build_lm_graph"
    generate();
    cout << "Landmarks generation time: " << lm_generation_timer << endl;
    if (lm_graph->number_of_landmarks() == 0)
        cout << "Warning! No landmarks found. Task unsolvable?" << endl;
    else {
        cout << "Discovered " << lm_graph->number_of_landmarks()
             << " landmarks, of which " << lm_graph->number_of_disj_landmarks()
             << " are disjunctive and "
             << lm_graph->number_of_conj_landmarks() << " are conjunctive \n"
             << lm_graph->number_of_edges() << " edges\n";
    }
    //lm_graph->dump();
    return lm_graph;
}

void LandmarkFactory::generate() {
    if (lm_graph->use_only_causal_landmarks())
        discard_noncausal_landmarks();
    if (!lm_graph->use_disjunctive_landmarks())
        discard_disjunctive_landmarks();
    if (!lm_graph->use_conjunctive_landmarks())
        discard_conjunctive_landmarks();
    lm_graph->set_landmark_ids();

    if (!lm_graph->use_orders())
        discard_all_orderings();
    else if (lm_graph->is_using_reasonable_orderings()) {
        cout << "approx. reasonable orders" << endl;
        approximate_reasonable_orders(false);
        cout << "approx. obedient reasonable orders" << endl;
        approximate_reasonable_orders(true);
    }
    mk_acyclic_graph();
    lm_graph->set_landmark_cost(calculate_lms_cost());
    calc_achievers();
}

bool LandmarkFactory::achieves_non_conditional(const Operator &o,
                                               const LandmarkNode *lmp) const {
    /* Test whether the landmark is achieved by the operator unconditionally.
    A disjunctive landmarks is achieved if one of its disjuncts is achieved. */
    assert(lmp != NULL);
    const vector<PrePost> &prepost = o.get_pre_post();
    for (unsigned j = 0; j < prepost.size(); j++) {
        for (unsigned int k = 0; k < lmp->vars.size(); k++) {
            if (prepost[j].var == lmp->vars[k] && prepost[j].post
                == lmp->vals[k])
                if (prepost[j].cond.empty())
                    return true;
        }
    }
    return false;
}

bool LandmarkFactory::is_landmark_precondition(const Operator &o,
                                               const LandmarkNode *lmp) const {
    /* Test whether the landmark is used by the operator as a precondition.
    A disjunctive landmarks is used if one of its disjuncts is used. */
    assert(lmp != NULL);
    const vector<Prevail> &prevail = o.get_prevail();
    for (unsigned j = 0; j < prevail.size(); j++) {
        for (unsigned int k = 0; k < lmp->vars.size(); k++) {
            if (prevail[j].var == lmp->vars[k] && prevail[j].prev
                == lmp->vals[k])
                //if (prepost[j].cond.empty())
                return true;
        }
    }
    const vector<PrePost> &prepost = o.get_pre_post();
    for (unsigned j = 0; j < prepost.size(); j++) {
        for (unsigned int k = 0; k < lmp->vars.size(); k++) {
            if (prepost[j].var == lmp->vars[k] && prepost[j].pre
                == lmp->vals[k])
                //if (prepost[j].cond.empty())
                return true;
        }
    }
    return false;
}

bool LandmarkFactory::relaxed_task_solvable(vector<vector<int> > &lvl_var,
                                            vector<hash_map<pair<int, int>, int, hash_int_pair> > &lvl_op,
                                            bool level_out, const LandmarkNode *exclude, bool compute_lvl_op) const {
    /* Test whether the relaxed planning task is solvable without achieving the propositions in
     "exclude" (do not apply operators that would add a proposition from "exclude").
     As a side effect, collect in lvl_var and lvl_op the earliest possible point in time
     when a proposition / operator can be achieved / become applicable in the relaxed task.
     */

    // Initialize lvl_op and lvl_var to numeric_limits<int>::max()
    if (compute_lvl_op) {
        lvl_op.resize(g_operators.size() + g_axioms.size());
        for (int i = 0; i < g_operators.size() + g_axioms.size(); i++) {
            const Operator &op = lm_graph->get_operator_for_lookup_index(i);
            lvl_op[i] = hash_map<pair<int, int>, int, hash_int_pair> ();
            const vector<PrePost> &prepost = op.get_pre_post();
            for (unsigned j = 0; j < prepost.size(); j++)
                lvl_op[i].insert(make_pair(make_pair(prepost[j].var,
                                                     prepost[j].post),
                                           numeric_limits<int>::max()));
        }
    }
    lvl_var.resize(g_variable_name.size());
    for (unsigned var = 0; var < g_variable_name.size(); var++) {
        lvl_var[var].resize(g_variable_domain[var],
                            numeric_limits<int>::max());
    }
    // Extract propositions from "exclude"
    hash_set<const Operator *, ex_hash_operator_ptr> exclude_ops;
    vector<pair<int, int> > exclude_props;
    if (exclude != NULL) {
        for (int op = 0; op < g_operators.size(); op++) {
            if (achieves_non_conditional(g_operators[op], exclude))
                exclude_ops.insert(&g_operators[op]);
        }
        for (int i = 0; i < exclude->vars.size(); i++)
            exclude_props.push_back(make_pair(exclude->vars[i],
                                              exclude->vals[i]));
    }
    // Do relaxed exploration
    lm_graph->get_exploration()->compute_reachability_with_excludes(lvl_var, lvl_op, level_out,
                                                                    exclude_props, exclude_ops, compute_lvl_op);

    // Test whether all goal propositions have a level of less than numeric_limits<int>::max()
    for (int i = 0; i < g_goal.size(); i++)
        if (lvl_var[g_goal[i].first][g_goal[i].second] ==
            numeric_limits<int>::max())
            return false;

    return true;
}


bool LandmarkFactory::is_causal_landmark(const LandmarkNode &landmark) const {
    /* Test whether the relaxed planning task is unsolvable without using any operator
       that has "landmark" has a precondition.
       Similar to "relaxed_task_solvable" above.
     */

    if (landmark.in_goal)
        return true;
    vector<vector<int> > lvl_var;
    vector<hash_map<pair<int, int>, int, hash_int_pair> > lvl_op;
    // Initialize lvl_var to numeric_limits<int>::max()
    lvl_var.resize(g_variable_name.size());
    for (unsigned var = 0; var < g_variable_name.size(); var++) {
        lvl_var[var].resize(g_variable_domain[var],
                            numeric_limits<int>::max());
    }
    hash_set<const Operator *, ex_hash_operator_ptr> exclude_ops;
    vector<pair<int, int> > exclude_props;
    for (int op = 0; op < g_operators.size(); op++) {
        if (is_landmark_precondition(g_operators[op], &landmark)) {
            exclude_ops.insert(&g_operators[op]);
        }
    }
    // Do relaxed exploration
    lm_graph->get_exploration()->compute_reachability_with_excludes(lvl_var, lvl_op, true,
                                                                    exclude_props, exclude_ops, false);

    // Test whether all goal propositions have a level of less than numeric_limits<int>::max()
    for (int i = 0; i < g_goal.size(); i++)
        if (lvl_var[g_goal[i].first][g_goal[i].second] ==
            numeric_limits<int>::max())
            return true;

    return false;
}

bool LandmarkFactory::effect_always_happens(const vector<PrePost> &prepost, set<
                                                pair<int, int> > &eff) const {
    /* Test whether the condition of a conditional effect is trivial, i.e. always true.
     We test for the simple case that the same effect proposition is triggered by
     a set of conditions of which one will always be true. This is e.g. the case in
     Schedule, where the effect
     (forall (?oldpaint - colour)
     (when (painted ?x ?oldpaint)
     (not (painted ?x ?oldpaint))))
     is translated by the translator to: if oldpaint == blue, then not painted ?x, and if
     oldpaint == red, then not painted ?x etc.
     If conditional effects are found that are always true, they are returned in "eff".
     */
    // Go through all effects of operator (as given by prepost) and collect:
    // - all variables that are set to some value in a conditional effect (effect_vars)
    // - variables that can be set to more than one value in a cond. effect (nogood_effect_vars)
    // - a mapping from cond. effect propositions to all the conditions that they appear with
    set<int> effect_vars;
    set<int> nogood_effect_vars;
    map<int, pair<int, vector<pair<int, int> > > > effect_conditions;
    for (unsigned i = 0; i < prepost.size(); i++) {
        if (prepost[i].cond.empty() ||
            nogood_effect_vars.find(prepost[i].var) != nogood_effect_vars.end()) {
            // Var has no condition or can take on different values, skipping
            continue;
        }
        if (effect_vars.find(prepost[i].var) != effect_vars.end()) {
            // We have seen this effect var before
            assert(effect_conditions.find(prepost[i].var) != effect_conditions.end());
            int old_eff = effect_conditions.find(prepost[i].var)->second.first;
            if (old_eff != prepost[i].post) {
                // Was different effect
                nogood_effect_vars.insert(prepost[i].var);
                continue;
            }
        } else {
            // We have not seen this effect var before
            effect_vars.insert(prepost[i].var);
        }
        if (effect_conditions.find(prepost[i].var) != effect_conditions.end()
            && effect_conditions.find(prepost[i].var)->second.first
            == prepost[i].post) {
            // We have seen this effect before, adding conditions
            for (int k = 0; k < prepost[i].cond.size(); k++) {
                vector<pair<int, int> > &vec = effect_conditions.find(prepost[i].var)->second.second;
                vec.push_back(make_pair(prepost[i].cond[k].var, prepost[i].cond[k].prev));
            }
        } else {
            // We have not seen this effect before, making new effect entry
            vector<pair<int, int> > &vec = effect_conditions.insert(make_pair(
                                                                        prepost[i].var, make_pair(prepost[i].post, vector<pair<int,
                                                                                                                               int> > ()))).first->second.second;
            for (int k = 0; k < prepost[i].cond.size(); k++) {
                vec.push_back(make_pair(prepost[i].cond[k].var, prepost[i].cond[k].prev));
            }
        }
    }

    // For all those effect propositions whose variables do not take on different values...
    map<int, pair<int, vector<pair<int, int> > > >::iterator it =
        effect_conditions.begin();
    for (; it != effect_conditions.end(); ++it) {
        if (nogood_effect_vars.find(it->first) != nogood_effect_vars.end()) {
            continue;
        }
        // ...go through all the conditions that the effect has, and map condition
        // variables to the set of values they take on (in unique_conds)
        map<int, set<int> > unique_conds;
        vector<pair<int, int> > &conds = it->second.second;
        for (unsigned int j = 0; j < conds.size(); j++) {
            if (unique_conds.find(conds[j].first) != unique_conds.end()) {
                unique_conds.find(conds[j].first)->second.insert(
                    conds[j].second);
            } else {
                set<int> &the_set = unique_conds.insert(make_pair(
                                                            conds[j].first, set<int> ())).first->second;
                the_set.insert(conds[j].second);
            }
        }
        // Check for each condition variable whether the number of values it takes on is
        // equal to the domain of that variable...
        pair<int, int> effect = make_pair(it->first, it->second.first);
        bool is_always_reached = true;
        map<int, set<int> >::iterator it2 = unique_conds.begin();
        for (; it2 != unique_conds.end(); ++it2) {
            bool is_surely_reached_by_var = false;
            if (it2->second.size() == g_variable_domain[it2->first]) {
                is_surely_reached_by_var = true;
            }
            // ...or else if the condition variable is the same as the effect variable,
            // check whether the condition variable takes on all other values except the
            // effect value
            else if (it2->first == it->first && it2->second.size()
                     == g_variable_domain[it2->first] - 1) {
                // Number of different values is correct, now ensure that the effect value
                // was the one missing
                it2->second.insert(it->second.first);
                if (it2->second.size() == g_variable_domain[it2->first]) {
                    is_surely_reached_by_var = true;
                }
            }
            // If one of the condition variables does not fulfill the criteria, the effect
            // is not certain to happen
            if (!is_surely_reached_by_var)
                is_always_reached = false;
        }
        if (is_always_reached)
            eff.insert(effect);
    }
    return eff.empty();
}

bool LandmarkFactory::interferes(const LandmarkNode *node_a,
                                 const LandmarkNode *node_b) const {
    /* Facts a and b interfere (i.e., achieving b before a would mean having to delete b
     and re-achieve it in order to achieve a) if one of the following condition holds:
     1. a and b are mutex
     2. All actions that add a also add e, and e and b are mutex
     3. There is a greedy necessary predecessor x of a, and x and b are mutex
     This is the definition of Hoffmann et al. except that they have one more condition:
     "all actions that add a delete b". However, in our case (SAS+ formalism), this condition
     is the same as 2.
     */
    assert(node_a != node_b);
    assert(!node_a->disjunctive && !node_b->disjunctive);

    for (int bi = 0; bi < node_b->vars.size(); bi++) {
        pair<int, int> b = make_pair(node_b->vars[bi], node_b->vals[bi]);
        for (int ai = 0; ai < node_a->vars.size(); ai++) {
            pair<int, int> a = make_pair(node_a->vars[ai], node_a->vals[ai]);

            if (a.first == b.first && a.second == b.second) {
                if (!node_a->conjunctive || !node_b->conjunctive)
                    return false;
                else
                    continue;
            }

            // 1. a, b mutex
            if (are_mutex(a, b))
                return true;

            // 2. Shared effect e in all operators reaching a, and e, b are mutex
            // Skip this for conjunctive nodes a, as they are typically achieved through a
            // sequence of operators successively adding the parts of a
            if (node_a->conjunctive)
                continue;

            hash_map<int, int> shared_eff;
            bool init = true;
            const vector<int> &ops = lm_graph->get_operators_including_eff(a);
            // Intersect operators that achieve a one by one
            for (unsigned i = 0; i < ops.size(); i++) {
                const Operator &op = lm_graph->get_operator_for_lookup_index(ops[i]);
                // If no shared effect among previous operators, break
                if (!init && shared_eff.empty())
                    break;
                // Else, insert effects of this operator into set "next_eff" if
                // it is an unconditional effect or a conditional effect that is sure to
                // happen. (Such "trivial" conditions can arise due to our translator,
                // e.g. in Schedule. There, the same effect is conditioned on a disjunction
                // of conditions of which one will always be true. We test for a simple kind
                // of these trivial conditions here.)
                const vector<PrePost> &prepost = op.get_pre_post();
                set<pair<int, int> > trivially_conditioned_effects;
                bool testing_for_trivial_conditions = true;
                bool trivial_conditioned_effects_found = false;
                if (testing_for_trivial_conditions)
                    trivial_conditioned_effects_found = effect_always_happens(prepost,
                                                                              trivially_conditioned_effects);
                hash_map<int, int> next_eff;
                for (unsigned i = 0; i < prepost.size(); i++) {
                    if (prepost[i].cond.empty() && prepost[i].var != a.first) {
                        next_eff.insert(make_pair(prepost[i].var, prepost[i].post));
                    } else if (testing_for_trivial_conditions
                               && trivial_conditioned_effects_found
                               && trivially_conditioned_effects.find(make_pair(
                                                                         prepost[i].var, prepost[i].post))
                               != trivially_conditioned_effects.end())
                        next_eff.insert(make_pair(prepost[i].var, prepost[i].post));
                }
                // Intersect effects of this operator with those of previous operators
                if (init)
                    swap(shared_eff, next_eff);
                else {
                    hash_map<int, int> result;
                    for (hash_map<int, int>::iterator it1 = shared_eff.begin(); it1
                         != shared_eff.end(); it1++) {
                        hash_map<int, int>::iterator it2 = next_eff.find(it1->first);
                        if (it2 != next_eff.end() && it2->second == it1->second)
                            result.insert(*it1);
                    }
                    swap(shared_eff, result);
                }
                init = false;
            }
            // Test whether one of the shared effects is inconsistent with b
            for (hash_map<int, int>::iterator it = shared_eff.begin(); it
                 != shared_eff.end(); it++)
                if (make_pair(it->first, it->second) != a && make_pair(it->first,
                                                                       it->second) != b && are_mutex(*it, b))
                    return true;
        }

        /* // Experimentally commenting this out -- see issue202.
        // 3. Exists LM x, inconsistent x, b and x->_gn a
        const LandmarkNode &node = *node_a;
        for (hash_map<LandmarkNode *, edge_type, hash_pointer>::const_iterator it =
                 node.parents.begin(); it != node.parents.end(); it++) {
            edge_type edge = it->second;
            for (int i = 0; i < it->first->vars.size(); i++) {
                pair<int, int> parent_prop = make_pair(it->first->vars[i],
                                                       it->first->vals[i]);
                if (edge >= greedy_necessary && parent_prop != b && are_mutex(
                        parent_prop, b))
                    return true;
            }
        }
        */
    }
    // No inconsistency found
    return false;
}

void LandmarkFactory::approximate_reasonable_orders(bool obedient_orders) {
    /* Approximate reasonable and obedient reasonable orders according to Hoffmann et al. If flag
    "obedient_orders" is true, we calculate obedient reasonable orders, otherwise reasonable orders.

    If node_p is in goal, then any node2_p which interferes with node_p can be reasonably ordered
    before node_p. Otherwise, if node_p is greedy necessary predecessor of node2, and there is another
    predecessor "parent" of node2, then parent and all predecessors of parent can be ordered reasonably
    before node_p if they interfere with node_p.
    */
    for (set<LandmarkNode *>::iterator it = lm_graph->get_nodes().begin(); it != lm_graph->get_nodes().end(); it++) {
        LandmarkNode *node_p = *it;
        if (node_p->disjunctive)
            continue;

        if (node_p->is_true_in_state(*g_initial_state))
            return;

        if (!obedient_orders && node_p->is_goal()) {
            for (set<LandmarkNode *>::iterator it2 = lm_graph->get_nodes().begin(); it2
                 != lm_graph->get_nodes().end(); it2++) {
                LandmarkNode *node2_p = *it2;
                if (node2_p == node_p || node2_p->disjunctive)
                    continue;
                if (interferes(node2_p, node_p)) {
                    edge_add(*node2_p, *node_p, reasonable);
                }
            }
        } else {
            // Collect candidates for reasonable orders in "interesting nodes".
            // Use hash set to filter duplicates.
            hash_set<LandmarkNode *, hash_pointer> interesting_nodes(
                g_variable_name.size());
            for (hash_map<LandmarkNode *, edge_type, hash_pointer>::iterator it =
                     node_p->children.begin(); it != node_p->children.end(); it++) {
                if (it->second >= greedy_necessary) { // found node2: node_p ->_gn node2
                    LandmarkNode &node2 = *(it->first);
                    for (hash_map<LandmarkNode *, edge_type, hash_pointer>::iterator
                         it2 = node2.parents.begin(); it2
                         != node2.parents.end(); it2++) {   // find parent
                        edge_type &edge = it2->second;
                        LandmarkNode &parent = *(it2->first);
                        if (parent.disjunctive)
                            continue;
                        if ((edge >= natural || (obedient_orders && edge == reasonable)) &&
                            &parent != node_p) {  // find predecessors or parent and collect in
                            // "interesting nodes"
                            interesting_nodes.insert(&parent);
                            collect_ancestors(interesting_nodes, parent,
                                              obedient_orders);
                        }
                    }
                }
            }
            // Insert reasonable orders between those members of "interesting nodes" that interfere
            // with node_p.
            for (hash_set<LandmarkNode *, hash_pointer>::iterator it3 =
                     interesting_nodes.begin(); it3 != interesting_nodes.end(); it3++) {
                if (*it3 == node_p || (*it3)->disjunctive)
                    continue;
                if (interferes(*it3, node_p)) {
                    if (!obedient_orders)
                        edge_add(**it3, *node_p, reasonable);
                    else
                        edge_add(**it3, *node_p, obedient_reasonable);
                }
            }
        }
    }
}

void LandmarkFactory::collect_ancestors(
    hash_set<LandmarkNode *, hash_pointer> &result, LandmarkNode &node,
    bool use_reasonable) {
    /* Returns all ancestors in the landmark graph of landmark node "start" */

    // There could be cycles if use_reasonable == true
    list<LandmarkNode *> open_nodes;
    hash_set<LandmarkNode *, hash_pointer> closed_nodes;
    for (hash_map<LandmarkNode *, edge_type, hash_pointer>::iterator it =
             node.parents.begin(); it != node.parents.end(); it++) {
        edge_type &edge = it->second;
        LandmarkNode &parent = *(it->first);
        if (edge >= natural || (use_reasonable && edge == reasonable))
            if (closed_nodes.find(&parent) == closed_nodes.end()) {
                open_nodes.push_back(&parent);
                closed_nodes.insert(&parent);
                result.insert(&parent);
            }

    }
    while (!open_nodes.empty()) {
        LandmarkNode &node2 = *(open_nodes.front());
        for (hash_map<LandmarkNode *, edge_type, hash_pointer>::iterator it =
                 node2.parents.begin(); it != node2.parents.end(); it++) {
            edge_type &edge = it->second;
            LandmarkNode &parent = *(it->first);
            if (edge >= natural || (use_reasonable && edge == reasonable)) {
                if (closed_nodes.find(&parent) == closed_nodes.end()) {
                    open_nodes.push_back(&parent);
                    closed_nodes.insert(&parent);
                    result.insert(&parent);
                }
            }
        }
        open_nodes.pop_front();
    }
}

void LandmarkFactory::edge_add(LandmarkNode &from, LandmarkNode &to,
                               edge_type type) {
    /* Adds an edge in the landmarks graph if there is no contradicting edge (simple measure to
    reduce cycles. If the edge is already present, the stronger edge type wins.
    */
    assert(&from != &to);
    assert(from.parents.find(&to) == from.parents.end() || type <= reasonable);
    assert(to.children.find(&from) == to.children.end() || type <= reasonable);

    if (type == reasonable || type == obedient_reasonable) { // simple cycle test
        if (from.parents.find(&to) != from.parents.end()) { // Edge in opposite direction exists
            //cout << "edge in opposite direction exists" << endl;
            if (from.parents.find(&to)->second > type) // Stronger order present, return
                return;
            // Edge in opposite direction is weaker, delete
            from.parents.erase(&to);
            to.children.erase(&from);
        }
    }

    // If edge already exists, remove if weaker
    if (from.children.find(&to) != from.children.end() && from.children.find(
            &to)->second < type) {
        from.children.erase(&to);
        assert(to.parents.find(&from) != to.parents.end());
        to.parents.erase(&from);

        assert(to.parents.find(&from) == to.parents.end());
        assert(from.children.find(&to) == from.children.end());
    }
    // If edge does not exist (or has just been removed), insert
    if (from.children.find(&to) == from.children.end()) {
        assert(to.parents.find(&from) == to.parents.end());
        from.children.insert(make_pair(&to, type));
        to.parents.insert(make_pair(&from, type));
        //cout << "added parent with address " << &from << endl;
    }
    assert(from.children.find(&to) != from.children.end());
    assert(to.parents.find(&from) != to.parents.end());
}

void LandmarkFactory::discard_noncausal_landmarks() {
    int number_of_noncausal_landmarks = 0;
    bool change = true;
    while (change) {
        change = false;
        for (set<LandmarkNode *>::const_iterator it = lm_graph->get_nodes().begin(); it
             != lm_graph->get_nodes().end(); ++it) {
            LandmarkNode *n = *it;
            if (!is_causal_landmark(*n)) {
                cout << "Discarding non-causal landmark: ";
                lm_graph->dump_node(n);
                lm_graph->rm_landmark_node(n);
                number_of_noncausal_landmarks++;
                change = true;
                break;
            }
        }
    }
    cout << "Discarded " << number_of_noncausal_landmarks
         << " non-causal landmarks" << endl;
}

void LandmarkFactory::discard_disjunctive_landmarks() {
    /* Using disjunctive landmarks during landmark generation can be
    beneficial even if we don't want to use disunctive landmarks during s
    search. This function deletes all disjunctive landmarks that have been
    found. (Note: this is implemented inefficiently because "nodes" contains
    pointers, not the actual nodes. We should change that.)
    */
    if (lm_graph->number_of_disj_landmarks() == 0)
        return;
    cout << "Discarding " << lm_graph->number_of_disj_landmarks()
         << " disjunctive landmarks" << endl;
    bool change = true;
    while (change) {
        change = false;
        for (set<LandmarkNode *>::const_iterator it = lm_graph->get_nodes().begin(); it
             != lm_graph->get_nodes().end(); ++it) {
            LandmarkNode *n = *it;
            if (n->disjunctive) {
                lm_graph->rm_landmark_node(n);
                change = true;
                break;
            }
        }
    }
    // [Malte] Commented out the following assertions because
    // the old methods for this are no longer available.
    // assert(lm_graph->number_of_disj_landmarks() == 0);
    // assert(disj_lms_to_nodes.size() == 0);
}

void LandmarkFactory::discard_conjunctive_landmarks() {
    if (lm_graph->number_of_conj_landmarks() == 0)
        return;
    cout << "Discarding " << lm_graph->number_of_conj_landmarks()
         << " conjunctive landmarks" << endl;
    bool change = true;
    while (change) {
        change = false;
        for (set<LandmarkNode *>::const_iterator it = lm_graph->get_nodes().begin(); it
             != lm_graph->get_nodes().end(); ++it) {
            LandmarkNode *n = *it;
            if (n->conjunctive) {
                lm_graph->rm_landmark_node(n);
                change = true;
                break;
            }
        }
    }
    // [Malte] Commented out the following assertion because
    // the old method for this is no longer available.
    // assert(number_of_conj_landmarks() == 0);
}

void LandmarkFactory::discard_all_orderings() {
    cout << "Removing all orderings." << endl;
    for (set<LandmarkNode *>::iterator it =
             lm_graph->get_nodes().begin(); it != lm_graph->get_nodes().end(); it++) {
        LandmarkNode &lmn = **it;
        lmn.children.clear();
        lmn.parents.clear();
    }
}

void LandmarkFactory::mk_acyclic_graph() {
    hash_set<LandmarkNode *, hash_pointer> acyclic_node_set(lm_graph->number_of_landmarks());
    int removed_edges = 0;
    for (set<LandmarkNode *>::iterator it = lm_graph->get_nodes().begin(); it != lm_graph->get_nodes().end(); it++) {
        LandmarkNode &lmn = **it;
        if (acyclic_node_set.find(&lmn) == acyclic_node_set.end())
            removed_edges += loop_acyclic_graph(lmn, acyclic_node_set);
    }
    // [Malte] Commented out the following assertion because
    // the old method for this is no longer available.
    // assert(acyclic_node_set.size() == number_of_landmarks());
    cout << "Removed " << removed_edges
         << " reasonable or obedient reasonable orders\n";
}

bool LandmarkFactory::remove_first_weakest_cycle_edge(LandmarkNode *cur,
                                                      list<pair<LandmarkNode *, edge_type> > &path, list<pair<LandmarkNode *,
                                                                                                              edge_type> >::iterator it) {
    LandmarkNode *parent_p = 0;
    LandmarkNode *child_p = 0;
    for (list<pair<LandmarkNode *, edge_type> >::iterator it2 = it; it2
         != path.end(); it2++) {
        edge_type edge = it2->second;
        if (edge == reasonable || edge == obedient_reasonable) {
            parent_p = it2->first;
            if (*it2 == path.back()) {
                child_p = cur;
                break;
            } else {
                list<pair<LandmarkNode *, edge_type> >::iterator child_it = it2;
                child_it++;
                child_p = child_it->first;
            }
            if (edge == obedient_reasonable)
                break;
            // else no break since o_r order could still appear in list
        }
    }
    assert(parent_p != 0 && child_p != 0);
    assert(parent_p->children.find(child_p) != parent_p->children.end());
    assert(child_p->parents.find(parent_p) != child_p->parents.end());
    parent_p->children.erase(child_p);
    child_p->parents.erase(parent_p);
    return true;
}

int LandmarkFactory::loop_acyclic_graph(LandmarkNode &lmn, hash_set<
                                            LandmarkNode *, hash_pointer> &acyclic_node_set) {
    assert(acyclic_node_set.find(&lmn) == acyclic_node_set.end());
    int nr_removed = 0;
    list<pair<LandmarkNode *, edge_type> > path;
    hash_set<LandmarkNode *, hash_pointer> visited = hash_set<LandmarkNode *,
                                                              hash_pointer> (lm_graph->number_of_landmarks());
    LandmarkNode *cur = &lmn;
    while (true) {
        assert(acyclic_node_set.find(cur) == acyclic_node_set.end());
        if (visited.find(cur) != visited.end()) { // cycle
            // find other occurence of cur node in path
            list<pair<LandmarkNode *, edge_type> >::iterator it;
            for (it = path.begin(); it != path.end(); it++) {
                if (it->first == cur)
                    break;
            }
            assert(it != path.end());
            // remove edge from graph
            remove_first_weakest_cycle_edge(cur, path, it);
            //assert(removed);
            nr_removed++;

            path.clear();
            cur = &lmn;
            visited.clear();
            continue;
        }
        visited.insert(cur);
        bool empty = true;
        for (hash_map<LandmarkNode *, edge_type, hash_pointer>::const_iterator
             it = cur->children.begin(); it != cur->children.end(); it++) {
            edge_type edge = it->second;
            LandmarkNode *child_p = it->first;
            if (acyclic_node_set.find(child_p) == acyclic_node_set.end()) {
                path.push_back(make_pair(cur, edge));
                cur = child_p;
                empty = false;
                break;
            }
        }
        if (!empty)
            continue;

        // backtrack
        visited.erase(cur);
        acyclic_node_set.insert(cur);
        if (!path.empty()) {
            cur = path.back().first;
            path.pop_back();
            visited.erase(cur);
        } else
            break;
    }
    assert(acyclic_node_set.find(&lmn) != acyclic_node_set.end());
    return nr_removed;
}

int LandmarkFactory::calculate_lms_cost() const {
    int result = 0;
    for (set<LandmarkNode *>::const_iterator it = lm_graph->get_nodes().begin(); it
         != lm_graph->get_nodes().end(); it++)
        result += (*it)->min_cost;

    return result;
}

void LandmarkFactory::compute_predecessor_information(
    LandmarkNode *bp,
    vector<vector<int> > &lvl_var, vector<hash_map<pair<int, int>, int,
                                                   hash_int_pair> > &lvl_op) {
    /* Collect information at what time step propositions can be reached
    (in lvl_var) in a relaxed plan that excludes bp, and similarly
    when operators can be applied (in lvl_op).  */

    relaxed_task_solvable(lvl_var, lvl_op, true, bp);
}

void LandmarkFactory::calc_achievers() {
    for (set<LandmarkNode *>::iterator node_it = lm_graph->get_nodes().begin(); node_it
         != lm_graph->get_nodes().end(); ++node_it) {
        LandmarkNode &lmn = **node_it;

        for (int k = 0; k < lmn.vars.size(); k++) {
            vector<int> ops = lm_graph->get_operators_including_eff(make_pair(
                                                                        lmn.vars[k], lmn.vals[k]));
            lmn.possible_achievers.insert(ops.begin(), ops.end());

            if (g_axiom_layers[lmn.vars[k]] != -1)
                lmn.is_derived = true;
        }

        vector<vector<int> > lvl_var;
        vector<hash_map<pair<int, int>, int, hash_int_pair> > lvl_op;
        compute_predecessor_information(&lmn, lvl_var, lvl_op);

        set<int>::iterator ach_it;
        for (ach_it = lmn.possible_achievers.begin(); ach_it
             != lmn.possible_achievers.end(); ++ach_it) {
            int op_id = *ach_it;
            const Operator &op = lm_graph->get_operator_for_lookup_index(op_id);

            if (_possibly_reaches_lm(op, lvl_var, &lmn)) {
                lmn.first_achievers.insert(op_id);
            }
        }
    }
}

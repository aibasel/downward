#include "landmark_graph_rpg_sasp.h"

#include "landmark_graph.h"
#include "util.h"

#include "../operator.h"
#include "../state.h"
#include "../globals.h"
#include "../domain_transition_graph.h"
#include "../option_parser.h"
#include "../plugin.h"

#include <cassert>
#include <limits>
#include <ext/hash_map>
#include <ext/hash_set>

using namespace __gnu_cxx;

static LandmarkGraphPlugin landmarks_graph_new_plugin(
    "lm_rhw", LandmarkGraphNew::create);
    
LandmarkGraphNew::LandmarkGraphNew(LandmarkGraph::Options &options, Exploration *exploration)
    : LandmarkFactory(options, exploration)  {
}

void LandmarkGraphNew::get_greedy_preconditions_for_lm(
    const LandmarkNode *lmp, const Operator &o, hash_map<int, int> &result) const {
    // Computes a subset of the actual preconditions of o for achieving lmp - takes into account
    // operator preconditions, but only reports those effect conditions that are true for ALL
    // effects achieving the LM.

    const vector<PrePost> &prepost = o.get_pre_post();
    for (unsigned j = 0; j < prepost.size(); j++) {
        if (prepost[j].pre != -1) {
            assert(prepost[j].pre < g_variable_domain[prepost[j].var]);
            result.insert(make_pair(prepost[j].var, prepost[j].pre));
        } else if (g_variable_domain[prepost[j].var] == 2) {
            for (int i = 0; i < lmp->vars.size(); i++) {
                if (lmp->vars[i] == prepost[j].var
                    && (*g_initial_state)[prepost[j].var] != lmp->vals[i]) {
                    result.insert(make_pair(prepost[j].var,
                                            (*g_initial_state)[prepost[j].var]));
                    break;
                }
            }
        }
    }

    const vector<Prevail> prevail = o.get_prevail();
    for (unsigned j = 0; j < prevail.size(); j++)
        result.insert(make_pair(prevail[j].var, prevail[j].prev));
    // Check for lmp in conditional effects
    set<int> lm_props_achievable;
    for (unsigned j = 0; j < prepost.size(); j++)
        for (int i = 0; i < lmp->vars.size(); i++)
            if (lmp->vars[i] == prepost[j].var && lmp->vals[i]
                == prepost[j].post)
                lm_props_achievable.insert(i);
    // Intersect effect conditions of all effects that can achieve lmp
    hash_map<int, int> intersection;
    bool init = true;
    set<int>::iterator it = lm_props_achievable.begin();
    for (; it != lm_props_achievable.end(); ++it) {
        for (unsigned j = 0; j < prepost.size(); j++) {
            if (!init && intersection.empty())
                break;
            hash_map<int, int> current_cond;
            if (lmp->vars[*it] == prepost[j].var && lmp->vals[*it]
                == prepost[j].post) {
                if (prepost[j].cond.empty()) {
                    intersection.clear();
                    break;
                } else
                    for (unsigned k = 0; k < prepost[j].cond.size(); k++)
                        current_cond.insert(make_pair(prepost[j].cond[k].var,
                                                      prepost[j].cond[k].prev));
            }
            if (init) {
                init = false;
                intersection = current_cond;
            } else
                intersection = _intersect(intersection, current_cond);
        }
    }
    result.insert(intersection.begin(), intersection.end());
}

int LandmarkGraphNew::min_cost_for_landmark(LandmarkNode *bp, vector<vector<
                                                                          int> > &lvl_var) {
    int min_cost = numeric_limits<int>::max();
    // For each proposition in bp...
    for (unsigned int k = 0; k < bp->vars.size(); k++) {
        pair<int, int> b = make_pair(bp->vars[k], bp->vals[k]);
        // ...look at all achieving operators
        const vector<int> &ops = lm_graph->get_operators_including_eff(b);
        for (unsigned i = 0; i < ops.size(); i++) {
            const Operator &op = lm_graph->get_operator_for_lookup_index(ops[i]);
            // and calculate the minimum cost of those that can make
            // bp true for the first time according to lvl_var
            if (_possibly_reaches_lm(op, lvl_var, bp))
                min_cost = min(min_cost, get_adjusted_action_cost(op, lm_graph->get_lm_cost_type()));
        }
    }
    assert(min_cost < numeric_limits<int>::max());
    return min_cost;
}

void LandmarkGraphNew::found_simple_lm_and_order(const pair<int, int> a,
                                                  LandmarkNode &b, edge_type t) {
    LandmarkNode *new_lm;
    if (lm_graph->simple_landmark_exists(a)) {
        new_lm = &lm_graph->get_simple_lm_node(a);
        edge_add(*new_lm, b, t);
        return;
    }
    set<pair<int, int> > a_set;
    a_set.insert(a);
    if (lm_graph->disj_landmark_exists(a_set)) {
        // Simple landmarks are more informative than disjunctive ones,
        // change disj. landmark into simple
        
        // TODO: can be removed when sure this is correct
        // old functionality
        /*LandmarkNode &node = lm_graph->get_disj_lm_node(a);
        node.disjunctive = false;
        for (int i = 0; i < node.vars.size(); i++)
            disj_lms_to_nodes.erase(make_pair(node.vars[i], node.vals[i]));
        simple_lms_to_nodes.insert(make_pair(a, &node));*/
        
        // new
        LandmarkNode &node = lm_graph->make_disj_node_simple(a);
        
        node.vars.clear();
        node.vals.clear();
        node.vars.push_back(a.first);
        node.vals.push_back(a.second);
        // Clean orders: let disj. LM {D1,...,Dn} be ordered before L. We
        // cannot infer that any one of D1,...Dn by itself is ordered before L
        for (hash_map<LandmarkNode *, edge_type, hash_pointer>::iterator it =
                 node.children.begin(); it != node.children.end(); it++)
            it->first->parents.erase(&node);
        node.children.clear();
        node.forward_orders.clear();

        edge_add(node, b, t);
        // Node has changed, reexamine it again. This also fixes min_cost.
        for (list<LandmarkNode *>::const_iterator it = open_landmarks.begin(); it
             != open_landmarks.end(); it++)
            if (*it == &node)
                return;
        open_landmarks.push_back(&node);
    } else {
        new_lm = &lm_graph->landmark_add_simple(a);
        open_landmarks.push_back(new_lm);
        edge_add(*new_lm, b, t);
    }
}

void LandmarkGraphNew::found_disj_lm_and_order(const set<pair<int, int> > a,
                                                LandmarkNode &b, edge_type t) {
    bool simple_lm_exists = false;
    pair<int, int> lm_prop;
    for (set<pair<int, int> >::iterator it = a.begin(); it != a.end(); ++it) {
        if ((*g_initial_state)[it->first] == it->second) {
            //cout << endl << "not adding LM that's true in initial state: "
            //<< g_variable_name[it->first] << " -> " << it->second << endl;
            return;
        }
        if (lm_graph->simple_landmark_exists(*it)) {
            // Propositions in this disj. LM exist already as simple LMs.
            simple_lm_exists = true;
            lm_prop = *it;
            break;
        }
    }
    LandmarkNode *new_lm;
    if (simple_lm_exists) {
        // Note: don't add orders as we can't be sure that they're correct
        return;
    } else if (lm_graph->disj_landmark_exists(a)) {
        if (lm_graph->exact_same_disj_landmark_exists(a)) {
            // LM already exists, just add order.
            new_lm = &lm_graph->get_disj_lm_node(*a.begin());
            edge_add(*new_lm, b, t);
            return;
        }
        // LM overlaps with existing disj. LM, do not add.
        return;
    }
    // This LM and no part of it exist, add the LM to the landmarks graph.
    new_lm = &lm_graph->landmark_add_disjunctive(a);
    open_landmarks.push_back(new_lm);
    edge_add(*new_lm, b, t);
}

void LandmarkGraphNew::compute_shared_preconditions(
    hash_map<int, int> &shared_pre, vector<vector<int> > &lvl_var,
    LandmarkNode *bp) {
    /* Compute the shared preconditions of all operators that can potentially
     achieve landmark bp, given lvl_var (reachability in relaxed planning graph) */
    bool init = true;
    for (unsigned int k = 0; k < bp->vars.size(); k++) {
        pair<int, int> b = make_pair(bp->vars[k], bp->vals[k]);
        const vector<int> &ops = lm_graph->get_operators_including_eff(b);

        for (unsigned i = 0; i < ops.size(); i++) {
            const Operator &op = lm_graph->get_operator_for_lookup_index(ops[i]);
            if (!init && shared_pre.empty())
                break;

            if (_possibly_reaches_lm(op, lvl_var, bp)) {
                hash_map<int, int> next_pre;
                get_greedy_preconditions_for_lm(bp, op, next_pre);
                if (init) {
                    init = false;
                    shared_pre = next_pre;
                } else
                    shared_pre = _intersect(shared_pre, next_pre);
            }
        }
    }
}

void LandmarkGraphNew::compute_disjunctive_preconditions(vector<set<pair<int,
                                                                          int> > > &disjunctive_pre, vector<vector<int> > &lvl_var,
                                                          LandmarkNode *bp) {
    /* Compute disjunctive preconditions from all operators than can potentially
     achieve landmark bp, given lvl_var (reachability in relaxed planning graph).
     A disj. precondition is a set of facts which contains one precondition fact
     from each of the operators, which we additionally restrict so that each fact
     in the set stems from the same PDDL predicate. */

    vector<int> ops;
    for (int i = 0; i < bp->vars.size(); i++) {
        pair<int, int> b = make_pair(bp->vars[i], bp->vals[i]);
        const vector<int> &tmp_ops = lm_graph->get_operators_including_eff(b);
        for (int j = 0; j < tmp_ops.size(); j++)
            ops.push_back(tmp_ops[j]);
    }
    int no_ops = 0;
    hash_map<int, vector<pair<int, int> > > preconditions; // maps from
    // pddl_proposition_indeces to props
    for (unsigned i = 0; i < ops.size(); i++) {
        const Operator &op = lm_graph->get_operator_for_lookup_index(ops[i]);
        if (_possibly_reaches_lm(op, lvl_var, bp)) {
            no_ops++;
            hash_map<int, int> next_pre;
            get_greedy_preconditions_for_lm(bp, op, next_pre);
            for (hash_map<int, int>::iterator it = next_pre.begin(); it
                 != next_pre.end(); it++) {
                hash_map<pair<int, int>, Pddl_proposition, hash_int_pair>::const_iterator
                    it2 = get_pddl_propositions().find(make_pair(it->first,
                                                           it->second));
                if (it2 == get_pddl_propositions().end()) // this can happen as translator
                    //introduces additional vars
                    continue;
                string pred = it2->second.predicate;
                int pred_index = get_pddl_proposition_indices().find(pred)->second;

                // Only deal with propositions that are not shared preconditions
                // (those have been found already and are simple landmarks).
                if (!lm_graph->simple_landmark_exists(make_pair(it->first, it->second))) {
                    if (preconditions.find(pred_index) == preconditions.end())
                        preconditions.insert(make_pair(pred_index, vector<pair<
                                                                              int, int> > ()));
                    preconditions.find(pred_index)->second.push_back(make_pair(
                                                                         it->first, it->second));
                }
            }
        }
    }
    for (hash_map<int, vector<pair<int, int> > >::iterator it =
             preconditions.begin(); it != preconditions.end(); ++it) {
        if (it->second.size() == no_ops) {
            set<pair<int, int> > pre_set; // the set gets rid of duplicate predicates
            pre_set.insert(it->second.begin(), it->second.end());
            if (pre_set.size() > 1) // otherwise this LM is not actually a disjunctive LM
                disjunctive_pre.push_back(pre_set);
        }
    }
}

void LandmarkGraphNew::generate_landmarks() {
    cout << "Generating landmarks using the RPG/SAS+ approach\n";
    for (unsigned i = 0; i < g_goal.size(); i++) {
        LandmarkNode &lmn = lm_graph->landmark_add_simple(g_goal[i]);
        lmn.in_goal = true;
        open_landmarks.push_back(&lmn);
    }

    while (!open_landmarks.empty()) {
        LandmarkNode *bp = open_landmarks.front();
        open_landmarks.pop_front();
        assert(bp->forward_orders.empty());

        if (!bp->is_true_in_state(*g_initial_state)) {
            // Backchain from landmark bp and compute greedy necessary predecessors.
            // Firstly, collect information about the earliest possible time step in a
            // relaxed plan that propositions are achieved (in lvl_var) and operators
            // applied (in lvl_ops).
            vector<vector<int> > lvl_var;
            vector<hash_map<pair<int, int>, int, hash_int_pair> > lvl_op;
            compute_predecessor_information(bp, lvl_var, lvl_op);
            // Use this information to determine all operators that can possibly achieve bp
            // for the first time, and collect any precondition propositions that all such
            // operators share (if there are any).
            hash_map<int, int> shared_pre;
            compute_shared_preconditions(shared_pre, lvl_var, bp);
            // All such shared preconditions are landmarks, and greedy necessary predecessors of bp.
            for (hash_map<int, int>::iterator it = shared_pre.begin(); it
                 != shared_pre.end(); it++) {
                found_simple_lm_and_order(*it, *bp, greedy_necessary);
            }
            // Extract additional orders from relaxed planning graph and DTG.
            approximate_lookahead_orders(lvl_var, bp);
            // Use the information about possibly achieving operators of bp to set its min cost.
            bp->min_cost = min_cost_for_landmark(bp, lvl_var);

            // Process achieving operators again to find disj. LMs
            vector<set<pair<int, int> > > disjunctive_pre;
            compute_disjunctive_preconditions(disjunctive_pre, lvl_var, bp);
            for (int i = 0; i < disjunctive_pre.size(); i++)
                if (disjunctive_pre[i].size() < 5) { // We don't want disj. LMs to get too big
                    found_disj_lm_and_order(disjunctive_pre[i], *bp, greedy_necessary);
                }

        }
    }
    add_lm_forward_orders();
}

void LandmarkGraphNew::approximate_lookahead_orders(
    const vector<vector<int> > &lvl_var, LandmarkNode *lmp) {
    // Find all var-val pairs that can only be reached after the landmark
    // (according to relaxed plan graph as captured in lvl_var)
    // the result is saved in the node member variable forward_orders, and will be
    // used later, when the phase of finding LMs has ended (because at the
    // moment we don't know which of these var-val pairs will be LMs).
    find_forward_orders(lvl_var, lmp);

    // Use domain transition graphs to find further orders. Only possible if lmp is
    // a simple landmark.
    if (lmp->disjunctive)
        return;
    const pair<int, int> lmk = make_pair(lmp->vars[0], lmp->vals[0]);

    // Collect in "unreached" all values of the LM variable that cannot be reached
    // before the LM value (in the relaxed plan graph)
    hash_set<int> unreached(g_variable_domain[lmk.first]);
    for (int i = 0; i < g_variable_domain[lmk.first]; i++)
        if (lvl_var[lmk.first][i] == numeric_limits<int>::max()
            && lmk.second != i)
            unreached.insert(i);
    // The set "exclude" will contain all those values of the LM variable that
    // cannot be reached before the LM value (as in "unreached") PLUS
    // one value that CAN be reached
    for (int i = 0; i < g_variable_domain[lmk.first]; i++)
        if (unreached.find(i) == unreached.end() && lmk.second != i) {
            hash_set<int> exclude(g_variable_domain[lmk.first]);
            exclude = unreached;
            exclude.insert(i);
            // If that value is crucial for achieving the LM from the initial state,
            // we have found a new landmark.
            if (!domain_connectivity(lmk, exclude))
                found_simple_lm_and_order(make_pair(lmk.first, i), *lmp, natural);
        }

}

bool LandmarkGraphNew::domain_connectivity(const pair<int, int> &landmark,
                                            const hash_set<int> &exclude) {
    /* Tests whether in the domain transition graph of the LM variable, there is
     a path from the initial state value to the LM value, without passing through
     any value in "exclude". If not, that means that one of the values in "exclude"
     is crucial for achieving the landmark (i.e. is on every path to the LM).
     */
    assert(landmark.second != (*g_initial_state)[landmark.first]); // no initial state landmarks
    // The value that we want to achieve must not be excluded:
    assert(exclude.find(landmark.second) == exclude.end());
    // If the value in the initial state is excluded, we won't achieve our goal value:
    if (exclude.find((*g_initial_state)[landmark.first]) != exclude.end())
        return false;
    list<int> open;
    hash_set<int> closed(g_variable_domain[landmark.first]);
    closed = exclude;
    open.push_back((*g_initial_state)[landmark.first]);
    closed.insert((*g_initial_state)[landmark.first]);
    while (closed.find(landmark.second) == closed.end()) {
        if (open.empty()) // landmark not in closed and nothing more to insert
            return false;
        const int c = open.front();
        open.pop_front();
        vector<int> succ;
        g_transition_graphs[landmark.first]->get_successors(c, succ);
        for (unsigned j = 0; j < succ.size(); j++)
            if (closed.find(succ[j]) == closed.end()) {
                open.push_back(succ[j]);
                closed.insert(succ[j]);
            }

    }
    return true;
}

void LandmarkGraphNew::find_forward_orders(
    const vector<vector<int> > &lvl_var, LandmarkNode *lmp) {
    /* lmp is ordered before any var-val pair that cannot be reached before lmp according to
     relaxed planning graph (as captured in lvl_var).
     These orders are saved in the node member variable "forward_orders".
     */
    for (int i = 0; i < g_variable_domain.size(); i++)
        for (int j = 0; j < g_variable_domain[i]; j++) {
            if (lvl_var[i][j] != numeric_limits<int>::max())
                continue;

            bool insert = true;
            for (int m = 0; m < lmp->vars.size() && insert; m++) {
                const pair<int, int> lm = make_pair(lmp->vars[m], lmp->vals[m]);

                if (make_pair(i, j) != lm) {
                    // Make sure there is no operator that reaches both lm and (i, j) at the same time
                    bool intersection_empty = true;
                    const vector<int> &reach_ij = lm_graph->get_operators_including_eff(
                        make_pair(i, j));
                    const vector<int> &reach_lm = lm_graph->get_operators_including_eff(
                        lm);

                    for (int k = 0; k < reach_ij.size() && intersection_empty; k++)
                        for (int l = 0; l < reach_lm.size()
                             && intersection_empty; l++)
                            if (reach_ij[k] == reach_lm[l])
                                intersection_empty = false;

                    if (!intersection_empty)
                        insert = false;
                } else
                    insert = false;
            }
            if (insert)
                lmp->forward_orders.insert(make_pair(i, j));
        }

}

void LandmarkGraphNew::add_lm_forward_orders() {
    set<LandmarkNode *>::const_iterator node_it;
    for (node_it = lm_graph->get_nodes().begin(); node_it != lm_graph->get_nodes().end(); ++node_it) {
        LandmarkNode &node = **node_it;

        for (hash_set<pair<int, int>, hash_int_pair>::iterator it2 =
                 node.forward_orders.begin(); it2 != node.forward_orders.end(); ++it2) {
            pair<int, int> node2_pair = *it2;

            if (lm_graph->simple_landmark_exists(node2_pair)) {
                LandmarkNode &node2 = lm_graph->get_simple_lm_node(node2_pair);
                edge_add(node, node2, natural);
            }
        }
        node.forward_orders.clear();
    }
}

LandmarkGraph *LandmarkGraphNew::create(
    const std::vector<string> &config, int start, int &end, bool dry_run) {
    LandmarkGraph::Options common_options;

    if (config.size() > start + 2 && config[start + 1] == "(") {
        end = start + 2;
        if (config[end] != ")") {
            NamedOptionParser option_parser;
            common_options.add_option_to_parser(option_parser);

            option_parser.parse_options(config, end, end, dry_run);
            end++;
        }
        if (config[end] != ")")
            throw ParseError(end);
    } else {
        end = start;
    }

    if (dry_run) {
        return 0;
    } else {
        LandmarkGraphNew lm_graph_factory(
            common_options,
            new Exploration(common_options.heuristic_options));
        LandmarkGraph *graph = lm_graph_factory.compute_lm_graph();
        return graph;
    }
}

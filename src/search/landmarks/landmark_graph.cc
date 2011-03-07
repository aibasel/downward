#include "landmark_graph.h"

#include "../exact_timer.h"
#include "../globals.h"
#include "../operator.h"
#include "../state.h"

#include <cassert>
#include <ext/hash_map>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

using namespace std;

LandmarkGraph::LandmarkGraph(Options &options, Exploration *explor)
    : exploration(explor), landmarks_count(0), conj_lms(0)/*,
      external_inconsistencies_read(false)*/ {
    reasonable_orders = options.reasonable_orders;
    only_causal_landmarks = options.only_causal_landmarks;
    disjunctive_landmarks = options.disjunctive_landmarks;
    conjunctive_landmarks = options.conjunctive_landmarks;
    no_orders = options.no_orders;
    if (options.lm_cost_type < 0 || options.lm_cost_type >= MAX_OPERATOR_COST) {
        cerr << "Illegal action cost type" << endl;
        exit(2);
    }
    lm_cost_type = static_cast<OperatorCost>(options.lm_cost_type);
    generate_operators_lookups();
}

void LandmarkGraph::generate_operators_lookups() {
    /* Build datastructures for efficient landmark computation. Map propositions
    to the operators that achieve them or have them as preconditions */
    
    operators_pre_lookup.resize(g_variable_domain.size());
    operators_eff_lookup.resize(g_variable_domain.size());
    for (unsigned i = 0; i < g_variable_domain.size(); i++) {
        operators_pre_lookup[i].resize(g_variable_domain[i]);
        operators_eff_lookup[i].resize(g_variable_domain[i]);
    }
    for (unsigned i = 0; i < g_operators.size() + g_axioms.size(); i++) {
        const Operator &op = get_operator_for_lookup_index(i);
        
        const vector<PrePost> &prepost = op.get_pre_post();
        bool no_pre = true;
        for (unsigned j = 0; j < prepost.size(); j++) {
            if (prepost[j].pre != -1) {
                no_pre = false;
                operators_pre_lookup[prepost[j].var][prepost[j].pre].push_back(
                i);
            }
            operators_eff_lookup[prepost[j].var][prepost[j].post].push_back(i);
        }
        const vector<Prevail> &prevail = op.get_prevail();
        for (unsigned j = 0; j < prevail.size(); j++) {
            no_pre = false;
            operators_pre_lookup[prevail[j].var][prevail[j].prev].push_back(i);
        }
        if (no_pre)
            empty_pre_operators.push_back(i);
    }
}

LandmarkNode *LandmarkGraph::landmark_reached(const pair<int, int> &prop) const {
    /* Return pointer to landmark node that corresponds to the given fact, or 0 if no such
     landmark exists. (TODO: rename to "get_landmark()")
     */
    LandmarkNode *node_p = 0;
    hash_map<pair<int, int>, LandmarkNode *, hash_int_pair>::const_iterator it =
        simple_lms_to_nodes.find(prop);
    if (it != simple_lms_to_nodes.end())
        node_p = it->second;
    else {
        hash_map<pair<int, int>, LandmarkNode *, hash_int_pair>::const_iterator
            it2 = disj_lms_to_nodes.find(prop);
        if (it2 != disj_lms_to_nodes.end())
            node_p = it2->second;
    }
    return node_p;
}

LandmarkNode *LandmarkGraph::get_lm_for_index(int i) {
    assert(ordered_nodes[i]->get_id() == i);
    return ordered_nodes[i];
}

inline static bool _in_goal(const pair<int, int> &l) {
    for (vector<pair<int, int> >::const_iterator start = g_goal.begin();
         start != g_goal.end(); start++)
        if (*start == l) // l \subseteq goal
            return true;

    return false;
}

int LandmarkGraph::number_of_edges() const {
    int total = 0;
    for (set<LandmarkNode *>::const_iterator it = nodes.begin(); it
         != nodes.end(); it++)
        total += (*it)->children.size();
    return total;
}

void LandmarkGraph::count_costs() {
    reached_cost = 0;
    needed_cost = 0;

    set<LandmarkNode *>::iterator node_it;
    for (node_it = nodes.begin(); node_it != nodes.end(); ++node_it) {
        LandmarkNode &node = **node_it;

        switch (node.status) {
        case lm_reached:
            reached_cost += node.min_cost;
            break;
        case lm_needed_again:
            reached_cost += node.min_cost;
            needed_cost += node.min_cost;
            break;
        case lm_not_reached:
            break;
        }
    }
}

/* Test whether the relaxed planning task is solvable without achieving the propositions in
"exclude" (do not apply operators that would add a proposition from "exclude").
As a side effect, collect in lvl_var and lvl_op the earliest possible point in time
when a proposition / operator can be achieved / become applicable in the relaxed task.
*/
/*bool LandmarkGraph::relaxed_task_solvable_without_operator(
    vector<vector<int> > &lvl_var, vector<hash_map<pair<int, int>, int,
                                                   hash_int_pair> > &lvl_op, bool level_out,
    const Operator *exclude, bool compute_lvl_op) const {

    // Initialize lvl_op and lvl_var to numeric_limits<int>::max()
    if (compute_lvl_op) {
        lvl_op.resize(g_operators.size() + g_axioms.size());
        for (int i = 0; i < g_operators.size() + g_axioms.size(); i++) {
            const Operator &op = get_operator_for_lookup_index(i);
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

    exclude_ops.insert(exclude);

    // Do relaxed exploration
    exploration->compute_reachability_with_excludes(lvl_var, lvl_op, level_out,
                                                    exclude_props, exclude_ops, compute_lvl_op);

    // Test whether all goal propositions have a level of less than
    // numeric_limits<int>::max()
    for (int i = 0; i < g_goal.size(); i++) {
        if (lvl_var[g_goal[i].first][g_goal[i].second] ==
            numeric_limits<int>::max()) {
            return false;
        }
    }
    return true;
}*/

bool LandmarkGraph::simple_landmark_exists(const pair<int, int> &lm) const {
    hash_map<pair<int, int>, LandmarkNode *, hash_int_pair>::const_iterator it =
    simple_lms_to_nodes.find(lm);
    assert(it == simple_lms_to_nodes.end() || !it->second->disjunctive);
    return it != simple_lms_to_nodes.end();
}

bool LandmarkGraph::landmark_exists(const pair<int, int> &lm) const {
    // Note: this only checks for one fact whether it's part of a landmark, hence only
    // simple and disjunctive landmarks are checked.
    set<pair<int, int> > lm_set;
    lm_set.insert(lm);
    return simple_landmark_exists(lm) || disj_landmark_exists(lm_set);
}

bool LandmarkGraph::disj_landmark_exists(const set<pair<int, int> > &lm) const {
    // Test whether ONE of the facts in lm is present in some disj. LM
    for (set<pair<int, int> >::const_iterator it = lm.begin(); it != lm.end(); ++it) {
        const pair<int, int> &prop = *it;
        hash_map<pair<int, int>, LandmarkNode *, hash_int_pair>::const_iterator
        it2 = disj_lms_to_nodes.find(prop);
        if (it2 != disj_lms_to_nodes.end())
            return true;
    }
    return false;
}

bool LandmarkGraph::exact_same_disj_landmark_exists(const set<pair<int, int> > &lm) const {
    // Test whether a disj. LM exists which consists EXACTLY of those facts in lm
    LandmarkNode *lmn = NULL;
    for (set<pair<int, int> >::const_iterator it = lm.begin(); it != lm.end(); ++it) {
        const pair<int, int> &prop = *it;
        hash_map<pair<int, int>, LandmarkNode *, hash_int_pair>::const_iterator
        it2 = disj_lms_to_nodes.find(prop);
        if (it2 == disj_lms_to_nodes.end())
            return false;
        else {
            if (lmn != NULL && lmn != it2->second) {
                return false;
            } else if (lmn == NULL)
                lmn = it2->second;
        }
    }
    return true;
}

LandmarkNode &LandmarkGraph::landmark_add_simple(const pair<int, int> &lm) {
    assert(!landmark_exists(lm));
    vector<int> vars;
    vector<int> vals;
    vars.push_back(lm.first);
    vals.push_back(lm.second);
    LandmarkNode *new_node_p = new LandmarkNode(vars, vals, false);
    nodes.insert(new_node_p);
    simple_lms_to_nodes.insert(make_pair(lm, new_node_p));
    landmarks_count++;
    return *new_node_p;
}

LandmarkNode &LandmarkGraph::landmark_add_disjunctive(
const set<pair<int, int> > &lm) {
    vector<int> vars;
    vector<int> vals;
    for (set<pair<int, int> >::iterator it = lm.begin(); it != lm.end(); ++it) {
        vars.push_back(it->first);
        vals.push_back(it->second);
        assert(!landmark_exists(make_pair(it->first, it->second)));
    }
    LandmarkNode *new_node_p = new LandmarkNode(vars, vals, true);
    new_node_p->disjunctive = true;
    nodes.insert(new_node_p);
    for (set<pair<int, int> >::iterator it = lm.begin(); it != lm.end(); ++it) {
        disj_lms_to_nodes.insert(make_pair(*it, new_node_p));
    }
    landmarks_count++;
    return *new_node_p;
}

void LandmarkGraph::insert_node(std::pair<int, int> lm, LandmarkNode &node, bool conj) {
    nodes.insert(&node);
    ++landmarks_count;
    if (conj) {
        ++conj_lms;
    } else {
        simple_lms_to_nodes.insert(std::make_pair(lm, &node));
    }
}

void LandmarkGraph::rm_landmark_node(LandmarkNode *node) {
    /* Called by "discard_disjunctive_landmarks()" */
    
    for (hash_map<LandmarkNode *, edge_type, hash_pointer>::iterator it =
        node->parents.begin(); it != node->parents.end(); it++) {
        LandmarkNode &parent = *(it->first);
    parent.children.erase(node);
    assert(parent.children.find(node) == parent.children.end());
    }
    for (hash_map<LandmarkNode *, edge_type, hash_pointer>::iterator it =
        node->children.begin(); it != node->children.end(); it++) {
        LandmarkNode &child = *(it->first);
    child.parents.erase(node);
    assert(child.parents.find(node) == child.parents.end());
    }
    if (node->disjunctive) {
        for (int i = 0; i < node->vars.size(); i++) {
            pair<int, int> lm = make_pair(node->vars[i], node->vals[i]);
            disj_lms_to_nodes.erase(lm);
        }
    } else if (node->conjunctive) {
        conj_lms--;
    } else {
        pair<int, int> lm = make_pair(node->vars[0], node->vals[0]);
        simple_lms_to_nodes.erase(lm);
    }
    nodes.erase(node);
    landmarks_count--;
    assert(nodes.find(node) == nodes.end());
}

void LandmarkGraph::rm_landmark(const pair<int, int> &lm) {
    assert(landmark_exists(lm));
    LandmarkNode *node;
    if (simple_landmark_exists(lm))
        node = &get_simple_lm_node(lm);
    else
        node = &get_disj_lm_node(lm);
    rm_landmark_node(node);
}

LandmarkNode &LandmarkGraph::make_disj_node_simple(std::pair<int, int> lm) {
    LandmarkNode &node = get_disj_lm_node(lm);
    node.disjunctive = false;
    for (int i = 0; i < node.vars.size(); i++)
        disj_lms_to_nodes.erase(make_pair(node.vars[i], node.vals[i]));
    simple_lms_to_nodes.insert(std::make_pair(lm, &node));
    return node;
}

void LandmarkGraph::set_landmark_ids() {
    ordered_nodes.resize(number_of_landmarks());
    int id = 0;
    for (set<LandmarkNode *>::iterator node_it =
        nodes.begin(); node_it != nodes.end(); node_it++) {
        LandmarkNode *lmn = *node_it;
    lmn->assign_id(id);
    ordered_nodes[id] = lmn;
    id++;
    }
}

void LandmarkGraph::dump_node(const LandmarkNode *node_p) const {
    cout << "LM " << node_p->get_id() << " ";
    if (node_p->disjunctive)
        cout << "disj {";
    else if (node_p->conjunctive)
        cout << "conj {";
    for (unsigned int i = 0; i < node_p->vars.size(); i++) {
        pair<int, int> node_prop = make_pair(node_p->vars[i], node_p->vals[i]);
        hash_map<pair<int, int>, vector<string>, hash_int_pair>::const_iterator
        it = var_val_to_predicates.find(node_prop);
        if (it != var_val_to_predicates.end()) {
            for (size_t j = 0; j < it->second.size(); ++j) {
                cout << it->second[j] << " ";
            }
            cout << "("
            << g_variable_name[node_prop.first] << "(" << node_prop.first << ")"
            << "->" << node_prop.second << ")";
        } else {
            cout << g_variable_name[node_prop.first] << " (" << node_prop.first << ") "
            << "->" << node_prop.second;
        }
        if (i < node_p->vars.size() - 1)
            cout << ", ";
    }
    if (node_p->disjunctive || node_p->conjunctive)
        cout << "}";
    if (node_p->in_goal)
        cout << "(goal)";
    cout << " Achievers (" << node_p->possible_achievers.size() << ", " << node_p->first_achievers.size() << ")";
    cout << endl;
}

void LandmarkGraph::dump() const {
    cout << "Landmark graph: " << endl;
    set<LandmarkNode *, LandmarkNodeComparer> nodes2(nodes.begin(), nodes.end());

    for (set<LandmarkNode *>::const_iterator it = nodes2.begin(); it
        != nodes2.end(); it++) {
        LandmarkNode *node_p = *it;
        dump_node(node_p);
        for (hash_map<LandmarkNode *, edge_type, hash_pointer>::const_iterator
            parent_it = node_p->parents.begin(); parent_it
            != node_p->parents.end(); parent_it++) {
            const edge_type &edge = parent_it->second;
            const LandmarkNode *parent_p = parent_it->first;
            cout << "\t\t<-_";
            switch (edge) {
                case necessary:
                    cout << "nec ";
                    break;
                case greedy_necessary:
                    cout << "gn  ";
                    break;
                case natural:
                    cout << "nat ";
                    break;
                case reasonable:
                    cout << "r   ";
                    break;
                case obedient_reasonable:
                    cout << "o_r ";
                    break;
            }
            dump_node(parent_p);
        }
        for (hash_map<LandmarkNode *, edge_type, hash_pointer>::const_iterator
            child_it = node_p->children.begin(); child_it
            != node_p->children.end(); child_it++) {
            const edge_type &edge = child_it->second;
            const LandmarkNode *child_p = child_it->first;
            cout << "\t\t->_";
            switch (edge) {
                case necessary:
                    cout << "nec ";
                    break;
                case greedy_necessary:
                    cout << "gn  ";
                    break;
                case natural:
                    cout << "nat ";
                    break;
                case reasonable:
                    cout << "r   ";
                    break;
                case obedient_reasonable:
                    cout << "o_r ";
                    break;
            }
            dump_node(child_p);
        }
    }
    cout << "Landmark graph end." << endl;
}

LandmarkGraph::Options::Options()
    : reasonable_orders(false),
      only_causal_landmarks(false),
      disjunctive_landmarks(true),
      conjunctive_landmarks(true),
      no_orders(false),
      lm_cost_type(NORMAL) {
}

void LandmarkGraph::Options::add_option_to_parser(NamedOptionParser &option_parser) {
    heuristic_options.add_option_to_parser(option_parser);
    // TODO: this is an ugly hack, which we need to pass the cost_type parameter to the
    // Exploration class, which is generated by the LandmarkGraph's create method

    option_parser.add_bool_option("reasonable_orders",
                                  &reasonable_orders,
                                  "generate reasonable orders");
    option_parser.add_bool_option("only_causal_landmarks",
                                  &only_causal_landmarks,
                                  "keep only causal landmarks");
    option_parser.add_bool_option("disjunctive_landmarks",
                                  &disjunctive_landmarks,
                                  "keep disjunctive landmarks");
    option_parser.add_bool_option("conjunctive_landmarks",
                                  &conjunctive_landmarks,
                                  "keep conjunctive landmarks");
    option_parser.add_bool_option("no_orders",
                                  &no_orders,
                                  "discard all orderings");
    option_parser.add_int_option("lm_cost_type",
                                 &lm_cost_type,
                                 "landmark action cost adjustment type");
}

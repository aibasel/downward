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

using namespace __gnu_cxx;
using namespace std;

LandmarkGraph::LandmarkGraph(const Options &opts)
    : exploration(opts.get<Exploration *>("explor")),
      landmarks_count(0), conj_lms(0) {
    reasonable_orders = opts.get<bool>("reasonable_orders");
    only_causal_landmarks = opts.get<bool>("only_causal_landmarks");
    disjunctive_landmarks = opts.get<bool>("disjunctive_landmarks");
    conjunctive_landmarks = opts.get<bool>("conjunctive_landmarks");
    no_orders = opts.get<bool>("no_orders");
    lm_cost_type = static_cast<OperatorCost>(opts.get_enum("lm_cost_type"));
    conditional_effects_supported = opts.get<bool>("supports_conditional_effects");
    generate_operators_lookups();
}

void LandmarkGraph::generate_operators_lookups() {
    /* Build datastructures for efficient landmark computation. Map propositions
    to the operators that achieve them or have them as preconditions */

    operators_eff_lookup.resize(g_variable_domain.size());
    for (unsigned i = 0; i < g_variable_domain.size(); i++) {
        operators_eff_lookup[i].resize(g_variable_domain[i]);
    }
    for (unsigned i = 0; i < g_operators.size() + g_axioms.size(); i++) {
        const Operator &op = get_operator_for_lookup_index(i);
        const vector<PrePost> &prepost = op.get_pre_post();
        for (unsigned j = 0; j < prepost.size(); j++) {
            operators_eff_lookup[prepost[j].var][prepost[j].post].push_back(i);
        }
    }
}

LandmarkNode *LandmarkGraph::get_landmark(const pair<int, int> &prop) const {
    /* Return pointer to landmark node that corresponds to the given fact, or 0 if no such
     landmark exists.
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

LandmarkNode &LandmarkGraph::landmark_add_disjunctive(const set<pair<int, int> > &lm) {
    vector<int> vars;
    vector<int> vals;
    for (set<pair<int, int> >::iterator it = lm.begin(); it != lm.end(); ++it) {
        vars.push_back(it->first);
        vals.push_back(it->second);
        assert(!landmark_exists(*it));
    }
    LandmarkNode *new_node_p = new LandmarkNode(vars, vals, true);
    nodes.insert(new_node_p);
    for (set<pair<int, int> >::iterator it = lm.begin(); it != lm.end(); ++it) {
        disj_lms_to_nodes.insert(make_pair(*it, new_node_p));
    }
    landmarks_count++;
    return *new_node_p;
}

LandmarkNode &LandmarkGraph::landmark_add_conjunctive(const set<pair<int, int> > &lm) {
    vector<int> vars;
    vector<int> vals;
    for (set<pair<int, int> >::iterator it = lm.begin(); it != lm.end(); ++it) {
        vars.push_back(it->first);
        vals.push_back(it->second);
        assert(!landmark_exists(*it));
    }
    LandmarkNode *new_node_p = new LandmarkNode(vars, vals, false, true);
    nodes.insert(new_node_p);
    ++landmarks_count;
    ++conj_lms;
    return *new_node_p;
}

void LandmarkGraph::rm_landmark_node(LandmarkNode *node) {
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

LandmarkNode &LandmarkGraph::make_disj_node_simple(pair<int, int> lm) {
    LandmarkNode &node = get_disj_lm_node(lm);
    node.disjunctive = false;
    for (int i = 0; i < node.vars.size(); i++)
        disj_lms_to_nodes.erase(make_pair(node.vars[i], node.vals[i]));
    simple_lms_to_nodes.insert(make_pair(lm, &node));
    return node;
}

void LandmarkGraph::set_landmark_ids() {
    ordered_nodes.resize(landmarks_count);
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
        int var_no = node_p->vars[i], value = node_p->vals[i];
        cout << g_fact_names[var_no][value] << " ("
             << g_variable_name[var_no] << "(" << var_no << ")"
             << "->" << value << ")";
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
        cout << endl;
    }
    cout << "Landmark graph end." << endl;
}

void LandmarkGraph::add_options_to_parser(OptionParser &parser) {
    Heuristic::add_options_to_parser(parser);
    parser.add_option<bool>("reasonable_orders",
                            "generate reasonable orders",
                            "false");
    parser.add_option<bool>("only_causal_landmarks",
                            "keep only causal landmarks",
                            "false");
    parser.add_option<bool>("disjunctive_landmarks",
                            "keep disjunctive landmarks",
                            "true");
    parser.add_option<bool>("conjunctive_landmarks",
                            "keep conjunctive landmarks",
                            "true");
    parser.add_option<bool>("no_orders",
                            "discard all orderings",
                            "false");

    /* TODO: The following lines overlap strongly with
       ::add_cost_type_option_to_parser, but the option name is
       different, so the method cannot be used directly. We could make
       the option name in ::add_cost_type_option_to_parser settable by
       the caller, but this doesn't seem worth it since this option
       should go away anyway once the landmark code is properly
       cleaned up. */
    vector<string> cost_types;
    cost_types.push_back("NORMAL");
    cost_types.push_back("ONE");
    cost_types.push_back("PLUSONE");
    parser.add_enum_option("lm_cost_type",
                           cost_types,
                           "landmark action cost adjustment",
                           "NORMAL");
}

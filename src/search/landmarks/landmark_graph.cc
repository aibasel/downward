#include "landmark_graph.h"

#include "../abstract_task.h"
#include "../global_operator.h"
#include "../global_state.h"
#include "../globals.h"

#include <cassert>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <vector>

using namespace std;

namespace landmarks {
LandmarkGraph::LandmarkGraph()
    : landmarks_count(0), conj_lms(0) {
    generate_operators_lookups();
}

void LandmarkGraph::generate_operators_lookups() {
    /* Build datastructures for efficient landmark computation. Map propositions
    to the operators that achieve them or have them as preconditions */

    operators_eff_lookup.resize(g_variable_domain.size());
    for (size_t i = 0; i < g_variable_domain.size(); ++i) {
        operators_eff_lookup[i].resize(g_variable_domain[i]);
    }
    for (size_t i = 0; i < g_operators.size() + g_axioms.size(); ++i) {
        const GlobalOperator &op = get_operator_for_lookup_index(i);
        const vector<GlobalEffect> &effects = op.get_effects();
        for (size_t j = 0; j < effects.size(); ++j) {
            operators_eff_lookup[effects[j].var][effects[j].val].push_back(i);
        }
    }
}

LandmarkNode *LandmarkGraph::get_landmark(const FactPair &fact) const {
    /* Return pointer to landmark node that corresponds to the given fact, or 0 if no such
     landmark exists.
     */
    LandmarkNode *node_p = 0;
    // TODO(issue635): Use FactPair struct for landmarks.
    pair<int, int> prop(fact.var, fact.value);
    auto it = simple_lms_to_nodes.find(prop);
    if (it != simple_lms_to_nodes.end())
        node_p = it->second;
    else {
        auto it2 = disj_lms_to_nodes.find(prop);
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
         != nodes.end(); ++it)
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
    auto it = simple_lms_to_nodes.find(lm);
    assert(it == simple_lms_to_nodes.end() || !it->second->disjunctive);
    return it != simple_lms_to_nodes.end();
}

bool LandmarkGraph::landmark_exists(const pair<int, int> &lm) const {
    // Note: this only checks for one fact whether it's part of a landmark, hence only
    // simple and disjunctive landmarks are checked.
    set<pair<int, int>> lm_set;
    lm_set.insert(lm);
    return simple_landmark_exists(lm) || disj_landmark_exists(lm_set);
}

bool LandmarkGraph::disj_landmark_exists(const set<pair<int, int>> &lm) const {
    // Test whether ONE of the facts in lm is present in some disj. LM
    for (const auto &prop : lm) {
        if (disj_lms_to_nodes.count(prop) == 1)
            return true;
    }
    return false;
}

bool LandmarkGraph::exact_same_disj_landmark_exists(const set<pair<int, int>> &lm) const {
    // Test whether a disj. LM exists which consists EXACTLY of those facts in lm
    LandmarkNode *lmn = NULL;
    for (const auto &prop : lm) {
        auto it2 = disj_lms_to_nodes.find(prop);
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
    ++landmarks_count;
    return *new_node_p;
}

LandmarkNode &LandmarkGraph::landmark_add_disjunctive(const set<pair<int, int>> &lm) {
    vector<int> vars;
    vector<int> vals;
    for (set<pair<int, int>>::iterator it = lm.begin(); it != lm.end(); ++it) {
        vars.push_back(it->first);
        vals.push_back(it->second);
        assert(!landmark_exists(*it));
    }
    LandmarkNode *new_node_p = new LandmarkNode(vars, vals, true);
    nodes.insert(new_node_p);
    for (set<pair<int, int>>::iterator it = lm.begin(); it != lm.end(); ++it) {
        disj_lms_to_nodes.insert(make_pair(*it, new_node_p));
    }
    ++landmarks_count;
    return *new_node_p;
}

LandmarkNode &LandmarkGraph::landmark_add_conjunctive(const set<pair<int, int>> &lm) {
    vector<int> vars;
    vector<int> vals;
    for (set<pair<int, int>>::iterator it = lm.begin(); it != lm.end(); ++it) {
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
    for (const auto &parent : node->parents) {
        LandmarkNode &parent_node = *(parent.first);
        parent_node.children.erase(node);
        assert(parent_node.children.find(node) == parent_node.children.end());
    }
    for (const auto &child : node->children) {
        LandmarkNode &child_node = *(child.first);
        child_node.parents.erase(node);
        assert(child_node.parents.find(node) == child_node.parents.end());
    }
    if (node->disjunctive) {
        for (size_t i = 0; i < node->vars.size(); ++i) {
            pair<int, int> lm = make_pair(node->vars[i], node->vals[i]);
            disj_lms_to_nodes.erase(lm);
        }
    } else if (node->conjunctive) {
        --conj_lms;
    } else {
        pair<int, int> lm = make_pair(node->vars[0], node->vals[0]);
        simple_lms_to_nodes.erase(lm);
    }
    nodes.erase(node);
    --landmarks_count;
    assert(nodes.find(node) == nodes.end());
}

LandmarkNode &LandmarkGraph::make_disj_node_simple(pair<int, int> lm) {
    LandmarkNode &node = get_disj_lm_node(lm);
    node.disjunctive = false;
    for (size_t i = 0; i < node.vars.size(); ++i)
        disj_lms_to_nodes.erase(make_pair(node.vars[i], node.vals[i]));
    simple_lms_to_nodes.insert(make_pair(lm, &node));
    return node;
}

void LandmarkGraph::set_landmark_ids() {
    ordered_nodes.resize(landmarks_count);
    int id = 0;
    for (set<LandmarkNode *>::iterator node_it =
             nodes.begin(); node_it != nodes.end(); ++node_it) {
        LandmarkNode *lmn = *node_it;
        lmn->assign_id(id);
        ordered_nodes[id] = lmn;
        ++id;
    }
}

void LandmarkGraph::dump_node(const LandmarkNode *node_p) const {
    cout << "LM " << node_p->get_id() << " ";
    if (node_p->disjunctive)
        cout << "disj {";
    else if (node_p->conjunctive)
        cout << "conj {";
    for (size_t i = 0; i < node_p->vars.size(); ++i) {
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

    for (const LandmarkNode *node_p : nodes2) {
        dump_node(node_p);
        for (const auto &parent : node_p->parents) {
            const LandmarkNode *parent_node = parent.first;
            const EdgeType &edge = parent.second;
            cout << "\t\t<-_";
            switch (edge) {
            case EdgeType::necessary:
                cout << "nec ";
                break;
            case EdgeType::greedy_necessary:
                cout << "gn  ";
                break;
            case EdgeType::natural:
                cout << "nat ";
                break;
            case EdgeType::reasonable:
                cout << "r   ";
                break;
            case EdgeType::obedient_reasonable:
                cout << "o_r ";
                break;
            }
            dump_node(parent_node);
        }
        for (const auto &child : node_p->children) {
            const LandmarkNode *child_node = child.first;
            const EdgeType &edge = child.second;
            cout << "\t\t->_";
            switch (edge) {
            case EdgeType::necessary:
                cout << "nec ";
                break;
            case EdgeType::greedy_necessary:
                cout << "gn  ";
                break;
            case EdgeType::natural:
                cout << "nat ";
                break;
            case EdgeType::reasonable:
                cout << "r   ";
                break;
            case EdgeType::obedient_reasonable:
                cout << "o_r ";
                break;
            }
            dump_node(child_node);
        }
        cout << endl;
    }
    cout << "Landmark graph end." << endl;
}
}

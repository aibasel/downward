#include "landmark_graph.h"

#include "util.h"

#include "../task_proxy.h"

#include <cassert>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <vector>

using namespace std;

namespace landmarks {
LandmarkGraph::LandmarkGraph(const TaskProxy &task_proxy)
    : landmarks_count(0), conj_lms(0) {
    generate_operators_lookups(task_proxy);
}

void LandmarkGraph::generate_operators_lookups(const TaskProxy &task_proxy) {
    /* Build datastructures for efficient landmark computation. Map propositions
    to the operators that achieve them or have them as preconditions */

    VariablesProxy variables = task_proxy.get_variables();
    operators_eff_lookup.resize(variables.size());
    for (VariableProxy var : variables) {
        operators_eff_lookup[var.get_id()].resize(var.get_domain_size());
    }
    OperatorsProxy operators = task_proxy.get_operators();
    for (OperatorProxy op : operators) {
        const EffectsProxy effects = op.get_effects();
        for (EffectProxy effect : effects) {
            const FactProxy effect_fact = effect.get_fact();
            operators_eff_lookup[effect_fact.get_variable().get_id()][effect_fact.get_value()].push_back(
                get_operator_or_axiom_id(op));
        }
    }
    for (OperatorProxy axiom : task_proxy.get_axioms()) {
        const EffectsProxy effects = axiom.get_effects();
        for (EffectProxy effect : effects) {
            const FactProxy effect_fact = effect.get_fact();
            operators_eff_lookup[effect_fact.get_variable().get_id()][effect_fact.get_value()].push_back(
                get_operator_or_axiom_id(axiom));
        }
    }
}

LandmarkNode *LandmarkGraph::get_landmark(const FactPair &fact) const {
    /* Return pointer to landmark node that corresponds to the given fact, or 0 if no such
     landmark exists.
     */
    LandmarkNode *node_p = nullptr;
    auto it = simple_lms_to_nodes.find(fact);
    if (it != simple_lms_to_nodes.end())
        node_p = it->second;
    else {
        auto it2 = disj_lms_to_nodes.find(fact);
        if (it2 != disj_lms_to_nodes.end())
            node_p = it2->second;
    }
    return node_p;
}

LandmarkNode *LandmarkGraph::get_lm_for_index(int i) const {
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

bool LandmarkGraph::simple_landmark_exists(const FactPair &lm) const {
    auto it = simple_lms_to_nodes.find(lm);
    assert(it == simple_lms_to_nodes.end() || !it->second->disjunctive);
    return it != simple_lms_to_nodes.end();
}

bool LandmarkGraph::landmark_exists(const FactPair &lm) const {
    // Note: this only checks for one fact whether it's part of a landmark, hence only
    // simple and disjunctive landmarks are checked.
    set<FactPair> lm_set;
    lm_set.insert(lm);
    return simple_landmark_exists(lm) || disj_landmark_exists(lm_set);
}

bool LandmarkGraph::disj_landmark_exists(const set<FactPair> &lm) const {
    // Test whether ONE of the facts in lm is present in some disj. LM
    for (const FactPair &lm_fact : lm) {
        if (disj_lms_to_nodes.count(lm_fact) == 1)
            return true;
    }
    return false;
}

bool LandmarkGraph::exact_same_disj_landmark_exists(const set<FactPair> &lm) const {
    // Test whether a disj. LM exists which consists EXACTLY of those facts in lm
    LandmarkNode *lmn = NULL;
    for (const FactPair &lm_fact : lm) {
        auto it2 = disj_lms_to_nodes.find(lm_fact);
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

LandmarkNode &LandmarkGraph::landmark_add_simple(const FactPair &lm) {
    assert(!landmark_exists(lm));
    vector<FactPair> facts;
    facts.push_back(lm);
    LandmarkNode *new_node_p = new LandmarkNode(facts, false);
    nodes.insert(new_node_p);
    simple_lms_to_nodes.emplace(lm, new_node_p);
    ++landmarks_count;
    return *new_node_p;
}

LandmarkNode &LandmarkGraph::landmark_add_disjunctive(const set<FactPair> &lm) {
    vector<FactPair> facts;
    for (const FactPair &lm_fact : lm) {
        facts.push_back(lm_fact);
        assert(!landmark_exists(lm_fact));
    }
    LandmarkNode *new_node_p = new LandmarkNode(facts, true);
    nodes.insert(new_node_p);
    for (const FactPair &lm_fact : lm) {
        disj_lms_to_nodes.emplace(lm_fact, new_node_p);
    }
    ++landmarks_count;
    return *new_node_p;
}

LandmarkNode &LandmarkGraph::landmark_add_conjunctive(const set<FactPair> &lm) {
    vector<FactPair> facts;
    for (const FactPair &lm_fact : lm) {
        facts.push_back(lm_fact);
        assert(!landmark_exists(lm_fact));
    }
    LandmarkNode *new_node_p = new LandmarkNode(facts, false, true);
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
        for (const FactPair &lm_fact : node->facts) {
            disj_lms_to_nodes.erase(lm_fact);
        }
    } else if (node->conjunctive) {
        --conj_lms;
    } else {
        simple_lms_to_nodes.erase(node->facts[0]);
    }
    nodes.erase(node);
    --landmarks_count;
    assert(nodes.find(node) == nodes.end());
}

LandmarkNode &LandmarkGraph::make_disj_node_simple(const FactPair &lm) {
    LandmarkNode &node = get_disj_lm_node(lm);
    node.disjunctive = false;
    for (const FactPair &lm_fact : node.facts)
        disj_lms_to_nodes.erase(lm_fact);
    simple_lms_to_nodes.emplace(lm, &node);
    return node;
}

void LandmarkGraph::set_landmark_ids() {
    ordered_nodes.resize(landmarks_count);
    int id = 0;
    for (LandmarkNode *lmn : nodes) {
        lmn->assign_id(id);
        ordered_nodes[id] = lmn;
        ++id;
    }
}

void LandmarkGraph::dump_node(const VariablesProxy &variables, const LandmarkNode *node_p) const {
    cout << "LM " << node_p->get_id() << " ";
    if (node_p->disjunctive)
        cout << "disj {";
    else if (node_p->conjunctive)
        cout << "conj {";
    size_t i = 0;
    for (const FactPair &lm_fact : node_p->facts) {
        VariableProxy var = variables[lm_fact.var];
        cout << var.get_fact(lm_fact.value).get_name() << " ("
             << var.get_name() << "(" << lm_fact.var << ")"
             << "->" << lm_fact.value << ")";
        if (i++ < node_p->facts.size() - 1)
            cout << ", ";
    }
    if (node_p->disjunctive || node_p->conjunctive)
        cout << "}";
    if (node_p->in_goal)
        cout << "(goal)";
    cout << " Achievers (" << node_p->possible_achievers.size() << ", " << node_p->first_achievers.size() << ")";
    cout << endl;
}

void LandmarkGraph::dump(const VariablesProxy &variables) const {
    cout << "Landmark graph: " << endl;
    set<LandmarkNode *, LandmarkNodeComparer> nodes2(nodes.begin(), nodes.end());

    for (const LandmarkNode *node_p : nodes2) {
        dump_node(variables, node_p);
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
            dump_node(variables, parent_node);
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
            dump_node(variables, child_node);
        }
        cout << endl;
    }
    cout << "Landmark graph end." << endl;
}
}

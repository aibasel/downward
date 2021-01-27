#include "landmark_graph.h"

#include "../utils/logging.h"
#include "../utils/memory.h"

#include <cassert>
#include <list>
#include <set>
#include <sstream>
#include <vector>

using namespace std;

namespace landmarks {
LandmarkGraph::LandmarkGraph(const TaskProxy &task_proxy)
    : conj_lms(0), disj_lms(0),
      task_proxy(task_proxy) {
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
    return nodes[i].get();
}

int LandmarkGraph::number_of_edges() const {
    int total = 0;
    for (auto &node : nodes)
        total += node->children.size();
    return total;
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
    LandmarkNode *lmn = nullptr;
    for (const FactPair &lm_fact : lm) {
        auto it2 = disj_lms_to_nodes.find(lm_fact);
        if (it2 == disj_lms_to_nodes.end())
            return false;
        else {
            if (lmn && lmn != it2->second) {
                return false;
            } else if (!lmn)
                lmn = it2->second;
        }
    }
    return true;
}

LandmarkNode &LandmarkGraph::landmark_add_simple(const FactPair &lm) {
    assert(!landmark_exists(lm));
    vector<FactPair> facts;
    facts.push_back(lm);
    unique_ptr<LandmarkNode> new_node = utils::make_unique_ptr<LandmarkNode>(facts, false);
    LandmarkNode *new_node_p = new_node.get();
    nodes.push_back(move(new_node));
    simple_lms_to_nodes.emplace(lm, new_node_p);
    return *new_node_p;
}

LandmarkNode &LandmarkGraph::landmark_add_disjunctive(const set<FactPair> &lm) {
    vector<FactPair> facts;
    for (const FactPair &lm_fact : lm) {
        facts.push_back(lm_fact);
        assert(!landmark_exists(lm_fact));
    }
    unique_ptr<LandmarkNode> new_node = utils::make_unique_ptr<LandmarkNode>(facts, true);
    LandmarkNode *new_node_p = new_node.get();
    nodes.push_back(move(new_node));
    for (const FactPair &lm_fact : lm) {
        disj_lms_to_nodes.emplace(lm_fact, new_node_p);
    }
    ++disj_lms;
    return *new_node_p;
}

LandmarkNode &LandmarkGraph::landmark_add_conjunctive(const set<FactPair> &lm) {
    vector<FactPair> facts;
    for (const FactPair &lm_fact : lm) {
        facts.push_back(lm_fact);
        assert(!landmark_exists(lm_fact));
    }
    unique_ptr<LandmarkNode> new_node = utils::make_unique_ptr<LandmarkNode>(facts, false, true);
    LandmarkNode *new_node_p = new_node.get();
    nodes.push_back(move(new_node));
    ++conj_lms;
    return *new_node_p;
}

void LandmarkGraph::remove_node_occurrences(LandmarkNode *node) {
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
        --disj_lms;
        for (const FactPair &lm_fact : node->facts) {
            disj_lms_to_nodes.erase(lm_fact);
        }
    } else if (node->conjunctive) {
        --conj_lms;
    } else {
        simple_lms_to_nodes.erase(node->facts[0]);
    }
}

void LandmarkGraph::remove_node_if(
    const function<bool (const LandmarkNode &)> &remove_node) {
    for (auto &node : nodes) {
        if (remove_node(*node)) {
            remove_node_occurrences(node.get());
        }
    }
    nodes.erase(remove_if(nodes.begin(), nodes.end(),
                          [&remove_node](const unique_ptr<LandmarkNode> &node) {
                              return remove_node(*node);
                          }), nodes.end());
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
    int id = 0;
    for (auto &lmn : nodes) {
        lmn->set_id(id);
        ++id;
    }
}

void LandmarkGraph::dump() const {
    utils::g_log << "Dump landmark graph: " << endl;

    cout << "digraph G {\n";
    for (const unique_ptr<LandmarkNode> &node : nodes) {
        dump_node(node);
        for (const auto &child : node->children) {
            const LandmarkNode *child_node = child.first;
            const EdgeType &edge = child.second;
            dump_edge(node->get_id(), child_node->get_id(), edge);
        }
    }
    cout << "}" << endl;
    utils::g_log << "Landmark graph end." << endl;
}

void LandmarkGraph::dump_node(const unique_ptr<LandmarkNode> &node) const {
    cout << "  lm" << node->get_id() << " [label=\"";

    FactPair &fact = node->facts[0];
    VariableProxy var = task_proxy.get_variables()[fact.var];
    cout << var.get_fact(fact.value).get_name();
    for (size_t i = 1; i < node->facts.size(); ++i) {
        if (node->disjunctive) {
            cout << " | ";
        } else if (node->conjunctive) {
            cout << " & ";
        }
        fact = node->facts[i];
        var = task_proxy.get_variables()[fact.var];
        cout << var.get_fact(fact.value).get_name();
    }
    cout << "\"";
    if (node->is_true_in_state(task_proxy.get_initial_state())) {
        cout << ", style=bold";
    }
    if (node->is_goal()) {
        cout << ", style=filled";
    }
    cout << "];\n";
}

void LandmarkGraph::dump_edge(int from, int to, EdgeType edge) const {
    cout << "      lm" << from << " -> lm" << to << " [label=";
    switch (edge) {
    case EdgeType::NECESSARY:
        cout << "\"nec\"";
        break;
    case EdgeType::GREEDY_NECESSARY:
        cout << "\"gn\"";
        break;
    case EdgeType::NATURAL:
        cout << "\"n\"";
        break;
    case EdgeType::REASONABLE:
        cout << "\"r\", style=dashed";
        break;
    case EdgeType::OBEDIENT_REASONABLE:
        cout << "\"o_r\", style=dashed";
        break;
    }
    cout << "];\n";
}
}

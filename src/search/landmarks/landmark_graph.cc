#include "landmark_graph.h"

#include "../utils/memory.h"

#include <cassert>
#include <list>
#include <set>
#include <sstream>
#include <vector>

using namespace std;

namespace landmarks {
bool LandmarkNode::is_true_in_state(const State &state) const {
    if (disjunctive) {
        for (const FactPair &fact : facts) {
            if (state[fact.var].get_value() == fact.value) {
                return true;
            }
        }
        return false;
    } else {
        // conjunctive or simple
        for (const FactPair &fact : facts) {
            if (state[fact.var].get_value() != fact.value) {
                return false;
            }
        }
        return true;
    }
}

LandmarkGraph::LandmarkGraph()
    : num_conjunctive_landmarks(0), num_disjunctive_landmarks(0) {
}

int LandmarkGraph::get_num_edges() const {
    int total = 0;
    for (auto &node : nodes)
        total += node->children.size();
    return total;
}

LandmarkNode *LandmarkGraph::get_landmark(int i) const {
    return nodes[i].get();
}

LandmarkNode *LandmarkGraph::get_landmark(const FactPair &fact) const {
    /* Return pointer to landmark node that corresponds to the given fact,
       or nullptr if no such landmark exists. */
    LandmarkNode *node_p = nullptr;
    auto it = simple_landmarks_to_nodes.find(fact);
    if (it != simple_landmarks_to_nodes.end())
        node_p = it->second;
    else {
        auto it2 = disjunctive_landmarks_to_nodes.find(fact);
        if (it2 != disjunctive_landmarks_to_nodes.end())
            node_p = it2->second;
    }
    return node_p;
}

LandmarkNode &LandmarkGraph::get_simple_landmark(const FactPair &fact) const {
    assert(contains_simple_landmark(fact));
    return *(simple_landmarks_to_nodes.find(fact)->second);
}

// needed only by landmarkgraph-factories.
LandmarkNode &LandmarkGraph::get_disjunctive_landmark(const FactPair &fact) const {
    /* Note: this only works because every proposition appears in only one
       disjunctive landmark. */
    assert(!contains_simple_landmark(fact));
    assert(contains_disjunctive_landmark(fact));
    return *(disjunctive_landmarks_to_nodes.find(fact)->second);
}


bool LandmarkGraph::contains_simple_landmark(const FactPair &lm) const {
    return simple_landmarks_to_nodes.count(lm) != 0;
}

bool LandmarkGraph::contains_disjunctive_landmark(const FactPair &lm) const {
    return disjunctive_landmarks_to_nodes.count(lm) != 0;
}

bool LandmarkGraph::contains_overlapping_disjunctive_landmark(
    const set<FactPair> &lm) const {
    // Test whether ONE of the facts is present in some disjunctive landmark.
    for (const FactPair &lm_fact : lm) {
        if (contains_disjunctive_landmark(lm_fact))
            return true;
    }
    return false;
}

bool LandmarkGraph::contains_identical_disjunctive_landmark(
    const set<FactPair> &lm) const {
    /* Test whether a disjunctive landmark exists which consists EXACTLY of
       the facts in lm. */
    LandmarkNode *lmn = nullptr;
    for (const FactPair &lm_fact : lm) {
        auto it2 = disjunctive_landmarks_to_nodes.find(lm_fact);
        if (it2 == disjunctive_landmarks_to_nodes.end())
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

bool LandmarkGraph::contains_landmark(const FactPair &lm) const {
    /* Note: this only checks for one fact whether it's part of a landmark,
       hence only simple and disjunctive landmarks are checked. */
    return contains_simple_landmark(lm) || contains_disjunctive_landmark(lm);
}

LandmarkNode &LandmarkGraph::add_simple_landmark(const FactPair &lm) {
    assert(!contains_landmark(lm));
    vector<FactPair> facts{lm};
    unique_ptr<LandmarkNode> new_node =
        utils::make_unique_ptr<LandmarkNode>(facts, false, false);
    LandmarkNode *new_node_p = new_node.get();
    nodes.push_back(move(new_node));
    simple_landmarks_to_nodes.emplace(lm, new_node_p);
    return *new_node_p;
}

LandmarkNode &LandmarkGraph::add_disjunctive_landmark(const set<FactPair> &lm) {
    assert(all_of(lm.begin(), lm.end(), [&](const FactPair &lm_fact) {
                      return !contains_landmark(lm_fact);
                  }));
    vector<FactPair> facts(lm.begin(), lm.end());
    unique_ptr<LandmarkNode> new_node =
        utils::make_unique_ptr<LandmarkNode>(facts, true, false);
    LandmarkNode *new_node_p = new_node.get();
    nodes.push_back(move(new_node));
    for (const FactPair &lm_fact : lm) {
        disjunctive_landmarks_to_nodes.emplace(lm_fact, new_node_p);
    }
    ++num_disjunctive_landmarks;
    return *new_node_p;
}

LandmarkNode &LandmarkGraph::add_conjunctive_landmark(const set<FactPair> &lm) {
    assert(all_of(lm.begin(), lm.end(), [&](const FactPair &lm_fact) {
                      return !contains_landmark(lm_fact);
                  }));
    vector<FactPair> facts(lm.begin(), lm.end());
    unique_ptr<LandmarkNode> new_node =
        utils::make_unique_ptr<LandmarkNode>(facts, false, true);
    LandmarkNode *new_node_p = new_node.get();
    nodes.push_back(move(new_node));
    ++num_conjunctive_landmarks;
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
        --num_disjunctive_landmarks;
        for (const FactPair &lm_fact : node->facts) {
            disjunctive_landmarks_to_nodes.erase(lm_fact);
        }
    } else if (node->conjunctive) {
        --num_conjunctive_landmarks;
    } else {
        simple_landmarks_to_nodes.erase(node->facts[0]);
    }
}

void LandmarkGraph::remove_node(LandmarkNode *node) {
    remove_node_occurrences(node);
    auto it = find_if(nodes.begin(), nodes.end(),
                      [&node](unique_ptr<LandmarkNode> &n) {
                          return n.get() == node;
                      });
    assert(it != nodes.end());
    nodes.erase(it);
}

void LandmarkGraph::remove_node_if(
    const function<bool (const LandmarkNode &)> &remove_node_condition) {
    for (auto &node : nodes) {
        if (remove_node_condition(*node)) {
            remove_node_occurrences(node.get());
        }
    }
    nodes.erase(remove_if(nodes.begin(), nodes.end(),
                          [&remove_node_condition](const unique_ptr<LandmarkNode> &node) {
                              return remove_node_condition(*node);
                          }), nodes.end());
}

void LandmarkGraph::set_landmark_ids() {
    int id = 0;
    for (auto &lmn : nodes) {
        lmn->set_id(id);
        ++id;
    }
}
}

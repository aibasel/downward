#include "landmark_graph.h"

#include "landmark.h"

#include "../utils/memory.h"

#include <cassert>
#include <list>
#include <set>
#include <sstream>
#include <vector>

using namespace std;

namespace landmarks {
LandmarkGraph::LandmarkGraph()
    : num_conjunctive_landmarks(0), num_disjunctive_landmarks(0) {
}

int LandmarkGraph::get_num_orderings() const {
    int total = 0;
    for (const auto &node : nodes)
        total += node->children.size();
    return total;
}

const LandmarkNode *LandmarkGraph::get_node(int i) const {
    return nodes[i].get();
}

LandmarkNode &LandmarkGraph::get_simple_landmark_node(
    const FactPair &atom) const {
    assert(contains_simple_landmark(atom));
    return *(simple_landmarks_to_nodes.find(atom)->second);
}

// needed only by landmarkgraph-factories.
LandmarkNode &LandmarkGraph::get_disjunctive_landmark_node(
    const FactPair &atom) const {
    /* Note: this only works because every proposition appears in only one
       disjunctive landmark. */
    assert(!contains_simple_landmark(atom));
    assert(contains_disjunctive_landmark(atom));
    return *(disjunctive_landmarks_to_nodes.find(atom)->second);
}


bool LandmarkGraph::contains_simple_landmark(const FactPair &atom) const {
    return simple_landmarks_to_nodes.count(atom) != 0;
}

bool LandmarkGraph::contains_disjunctive_landmark(const FactPair &atom) const {
    return disjunctive_landmarks_to_nodes.count(atom) != 0;
}

bool LandmarkGraph::contains_overlapping_disjunctive_landmark(
    const set<FactPair> &atoms) const {
    // Test whether ONE of the atoms is present in some disjunctive landmark.
    for (const FactPair &atom : atoms) {
        if (contains_disjunctive_landmark(atom))
            return true;
    }
    return false;
}

bool LandmarkGraph::contains_identical_disjunctive_landmark(
    const set<FactPair> &atoms) const {
    LandmarkNode *node = nullptr;
    for (const FactPair &atom : atoms) {
        auto it = disjunctive_landmarks_to_nodes.find(atom);
        if (it == disjunctive_landmarks_to_nodes.end())
            return false;
        else {
            if (node && node != it->second) {
                return false;
            } else if (!node)
                node = it->second;
        }
    }
    return true;
}

bool LandmarkGraph::contains_landmark(const FactPair &atom) const {
    /* Note: this only checks for one atom whether it's part of a landmark,
       hence only simple and disjunctive landmarks are checked. */
    return contains_simple_landmark(atom) ||
           contains_disjunctive_landmark(atom);
}

LandmarkNode *LandmarkGraph::add_node(Landmark &&landmark) {
    unique_ptr<LandmarkNode> new_node =
        utils::make_unique_ptr<LandmarkNode>(move(landmark));
    nodes.push_back(move(new_node));
    return nodes.back().get();
}

LandmarkNode &LandmarkGraph::add_landmark(Landmark &&landmark_to_add) {
    assert(landmark_to_add.is_conjunctive
           || all_of(landmark_to_add.atoms.begin(), landmark_to_add.atoms.end(),
                     [&](const FactPair &atom) {
                         return !contains_landmark(atom);
                     }));
    /*
      TODO: Avoid having to fetch landmark after moving it. This will only be
      possible after removing the assumption that landmarks don't overlap
      because we wont need `disjunctive_landmarks_to_nodes` and
      `simple_landmarks_to_nodes` anymore.
    */
    LandmarkNode *new_node = add_node(move(landmark_to_add));
    const Landmark &landmark = new_node->get_landmark();

    if (landmark.is_disjunctive) {
        for (const FactPair &atom : landmark.atoms) {
            disjunctive_landmarks_to_nodes.emplace(atom, new_node);
        }
        ++num_disjunctive_landmarks;
    } else if (landmark.is_conjunctive) {
        ++num_conjunctive_landmarks;
    } else {
        simple_landmarks_to_nodes.emplace(landmark.atoms.front(), new_node);
    }
    return *new_node;
}

void LandmarkGraph::remove_node_occurrences(LandmarkNode *node) {
    for (const auto &[parent, type] : node->parents) {
        parent->children.erase(node);
        assert(parent->children.find(node) == parent->children.end());
    }
    for (const auto &[child, type] : node->children) {
        child->parents.erase(node);
        assert(child->parents.find(node) == child->parents.end());
    }
    const Landmark &landmark = node->get_landmark();
    if (landmark.is_disjunctive) {
        --num_disjunctive_landmarks;
        for (const FactPair &atom : landmark.atoms) {
            disjunctive_landmarks_to_nodes.erase(atom);
        }
    } else if (landmark.is_conjunctive) {
        --num_conjunctive_landmarks;
    } else {
        simple_landmarks_to_nodes.erase(landmark.atoms[0]);
    }
}

void LandmarkGraph::remove_node(LandmarkNode *node) {
    remove_node_occurrences(node);
    auto it = find_if(nodes.begin(), nodes.end(),
                      [&node](const auto &other) {
                          return other.get() == node;
                      });
    assert(it != nodes.end());
    nodes.erase(it);
}

void LandmarkGraph::remove_node_if(
    const function<bool (const LandmarkNode &)> &remove_node_condition) {
    for (const auto &node : nodes) {
        if (remove_node_condition(*node)) {
            remove_node_occurrences(node.get());
        }
    }
    nodes.erase(remove_if(nodes.begin(), nodes.end(),
                          [&remove_node_condition](
                              const unique_ptr<LandmarkNode> &node) {
                              return remove_node_condition(*node);
                          }), nodes.end());
}

void LandmarkGraph::set_landmark_ids() {
    int id = 0;
    for (const auto &node : nodes) {
        node->set_id(id);
        ++id;
    }
}
}

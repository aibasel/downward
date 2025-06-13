#include "landmark_graph.h"

#include "landmark.h"

#include <cassert>
#include <ranges>
#include <vector>

using namespace std;

namespace landmarks {
LandmarkGraph::LandmarkGraph()
    : num_conjunctive_landmarks(0), num_disjunctive_landmarks(0) {
}

int LandmarkGraph::get_num_orderings() const {
    int total = 0;
    for (const auto &node : nodes) {
        total += static_cast<int>(node->children.size());
    }
    return total;
}

const LandmarkNode *LandmarkGraph::get_node(int i) const {
    return nodes[i].get();
}

LandmarkNode &LandmarkGraph::get_atomic_landmark_node(
    const FactPair &atom) const {
    assert(contains_atomic_landmark(atom));
    return *(atomic_landmarks_to_nodes.find(atom)->second);
}

LandmarkNode &LandmarkGraph::get_disjunctive_landmark_node(
    const FactPair &atom) const {
    /* Note: this only works because every proposition appears in only one
       disjunctive landmark. */
    assert(!contains_atomic_landmark(atom));
    assert(contains_disjunctive_landmark(atom));
    return *(disjunctive_landmarks_to_nodes.find(atom)->second);
}


bool LandmarkGraph::contains_atomic_landmark(const FactPair &atom) const {
    return atomic_landmarks_to_nodes.contains(atom);
}

bool LandmarkGraph::contains_disjunctive_landmark(const FactPair &atom) const {
    return disjunctive_landmarks_to_nodes.contains(atom);
}

bool LandmarkGraph::contains_overlapping_disjunctive_landmark(
    const utils::HashSet<FactPair> &atoms) const {
    return ranges::any_of(atoms, [&](const FactPair &atom) {
                              return contains_disjunctive_landmark(atom);
                          });
}

bool LandmarkGraph::contains_superset_disjunctive_landmark(
    const utils::HashSet<FactPair> &atoms) const {
    assert(!atoms.empty());
    const LandmarkNode *node = nullptr;
    for (const FactPair &atom : atoms) {
        auto it = disjunctive_landmarks_to_nodes.find(atom);
        if (it == disjunctive_landmarks_to_nodes.end()) {
            return false;
        }
        if (!node) {
            node = it->second;
        } else if (node != it->second) {
            return false;
        }
    }
    assert(node);
    return true;
}

bool LandmarkGraph::contains_landmark(const FactPair &atom) const {
    /* Note: this only checks for one atom whether it's part of a landmark,
       hence only atomic and disjunctive landmarks are checked. */
    return contains_atomic_landmark(atom) ||
           contains_disjunctive_landmark(atom);
}

LandmarkNode *LandmarkGraph::add_node(Landmark &&landmark) {
    unique_ptr<LandmarkNode> new_node =
        make_unique<LandmarkNode>(move(landmark));
    nodes.push_back(move(new_node));
    return nodes.back().get();
}

LandmarkNode &LandmarkGraph::add_landmark(Landmark &&landmark_to_add) {
    assert(landmark_to_add.type == CONJUNCTIVE || ranges::all_of(
               landmark_to_add.atoms, [&](const FactPair &atom) {
                   return !contains_landmark(atom);
               }));
    /*
      TODO: Avoid having to fetch landmark after moving it. This will only be
      possible after removing the assumption that landmarks don't overlap
      (issue257) because we wont need `disjunctive_landmarks_to_nodes` and
      `atomic_landmarks_to_nodes` anymore.
    */
    LandmarkNode *new_node = add_node(move(landmark_to_add));
    const Landmark &landmark = new_node->get_landmark();

    switch (landmark.type) {
    case DISJUNCTIVE:
        for (const FactPair &atom : landmark.atoms) {
            disjunctive_landmarks_to_nodes.emplace(atom, new_node);
        }
        ++num_disjunctive_landmarks;
        break;
    case ATOMIC:
        atomic_landmarks_to_nodes.emplace(landmark.atoms.front(), new_node);
        break;
    case CONJUNCTIVE:
        ++num_conjunctive_landmarks;
        break;
    }
    return *new_node;
}

void LandmarkGraph::remove_node_occurrences(LandmarkNode *node) {
    for (LandmarkNode *parent : views::keys(node->parents)) {
        parent->children.erase(node);
        assert(!parent->children.contains(node));
    }
    for (LandmarkNode *child : views::keys(node->children)) {
        child->parents.erase(node);
        assert(!child->parents.contains(node));
    }
    const Landmark &landmark = node->get_landmark();
    switch (landmark.type) {
    case DISJUNCTIVE:
        --num_disjunctive_landmarks;
        for (const FactPair &atom : landmark.atoms) {
            disjunctive_landmarks_to_nodes.erase(atom);
        }
        break;
    case ATOMIC:
        atomic_landmarks_to_nodes.erase(landmark.atoms[0]);
        break;
    case CONJUNCTIVE:
        --num_conjunctive_landmarks;
        break;
    }
}

void LandmarkGraph::remove_node(LandmarkNode *node) {
    remove_node_occurrences(node);
    const auto it =
        ranges::find_if(nodes, [&node](const auto &other) {
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
    erase_if(nodes,
             [&remove_node_condition](const unique_ptr<LandmarkNode> &node) {
                 return remove_node_condition(*node);
             });
}

void LandmarkGraph::set_landmark_ids() {
    int id = 0;
    for (const auto &node : nodes) {
        node->set_id(id);
        ++id;
    }
}
}

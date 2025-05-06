#include "utils_landmarks.h"

#include "../plugins/plugin.h"
#include "../landmarks/landmark.h"
#include "../landmarks/landmark_factory_hm.h"
#include "../landmarks/landmark_graph.h"
#include "../utils/logging.h"

#include <algorithm>
#include <ranges>
#include <unordered_set>

using namespace std;
using namespace landmarks;

namespace cartesian_abstractions {
static FactPair get_atom(const Landmark &landmark) {
    // We assume that the given Landmarks are from an h^m landmark graph with m=1.
    assert(landmark.type == ATOMIC);
    assert(landmark.atoms.size() == 1);
    return landmark.atoms[0];
}

shared_ptr<LandmarkGraph> get_landmark_graph(
    const shared_ptr<AbstractTask> &task) {
    LandmarkFactoryHM landmark_graph_factory(
        1, false, true, utils::Verbosity::SILENT);

    return landmark_graph_factory.compute_landmark_graph(task);
}

vector<FactPair> get_atom_landmarks(const LandmarkGraph &graph) {
    vector<FactPair> atoms;
    atoms.reserve(graph.get_num_landmarks());
    for (const auto &node : graph) {
        atoms.push_back(get_atom(node->get_landmark()));
    }
    sort(atoms.begin(), atoms.end());
    return atoms;
}

utils::HashMap<FactPair, LandmarkNode *> get_atom_to_landmark_map(
    const shared_ptr<LandmarkGraph> &graph) {
    // All landmarks are atomic, i.e., each has exactly one atom.
    assert(all_of(graph->begin(), graph->end(), [](auto &node) {
                      return node->get_landmark().atoms.size() == 1;
                  }));
    utils::HashMap<FactPair, landmarks::LandmarkNode *> atom_to_landmark_map;
    for (const auto &node : *graph) {
        const FactPair &atom = node->get_landmark().atoms[0];
        atom_to_landmark_map[atom] = node.get();
    }
    return atom_to_landmark_map;
}

VarToValues get_prev_landmarks(const LandmarkNode *node) {
    VarToValues groups;
    vector<const LandmarkNode *> open;
    unordered_set<const LandmarkNode *> closed;
    open.reserve(node->parents.size());
    for (const LandmarkNode *parent : views::keys(node->parents)) {
        open.push_back(parent);
    }
    while (!open.empty()) {
        const LandmarkNode *ancestor = open.back();
        open.pop_back();
        if (closed.contains(ancestor)) {
            continue;
        }
        closed.insert(ancestor);
        FactPair ancestor_atom = get_atom(ancestor->get_landmark());
        groups[ancestor_atom.var].push_back(ancestor_atom.value);
        for (const LandmarkNode *parent : views::keys(ancestor->parents)) {
            open.push_back(parent);
        }
    }
    return groups;
}
}

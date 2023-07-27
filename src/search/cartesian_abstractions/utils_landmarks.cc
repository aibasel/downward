#include "utils_landmarks.h"

#include "../plugins/plugin.h"
#include "../landmarks/landmark.h"
#include "../landmarks/landmark_factory_h_m.h"
#include "../landmarks/landmark_graph.h"
#include "../utils/logging.h"
#include "../utils/memory.h"

#include <algorithm>

using namespace std;
using namespace landmarks;

namespace cartesian_abstractions {
static FactPair get_fact(const Landmark &landmark) {
    // We assume that the given Landmarks are from an h^m landmark graph with m=1.
    assert(landmark.facts.size() == 1);
    return landmark.facts[0];
}

shared_ptr<LandmarkGraph> get_landmark_graph(const shared_ptr<AbstractTask> &task) {
    plugins::Options hm_opts;
    hm_opts.set<int>("m", 1);
    hm_opts.set<bool>("only_causal_landmarks", false);
    hm_opts.set<bool>("conjunctive_landmarks", false);
    hm_opts.set<bool>("use_orders", true);
    hm_opts.set<utils::Verbosity>("verbosity", utils::Verbosity::SILENT);
    LandmarkFactoryHM lm_graph_factory(hm_opts);

    return lm_graph_factory.compute_lm_graph(task);
}

vector<FactPair> get_fact_landmarks(const LandmarkGraph &graph) {
    vector<FactPair> facts;
    const LandmarkGraph::Nodes &nodes = graph.get_nodes();
    facts.reserve(nodes.size());
    for (auto &node : nodes) {
        facts.push_back(get_fact(node->get_landmark()));
    }
    sort(facts.begin(), facts.end());
    return facts;
}

VarToValues get_prev_landmarks(const LandmarkGraph &graph, const FactPair &fact) {
    VarToValues groups;
    LandmarkNode *node = graph.get_node(fact);
    assert(node);
    vector<const LandmarkNode *> open;
    unordered_set<const LandmarkNode *> closed;
    for (const auto &parent_and_edge : node->parents) {
        const LandmarkNode *parent = parent_and_edge.first;
        open.push_back(parent);
    }
    while (!open.empty()) {
        const LandmarkNode *ancestor = open.back();
        open.pop_back();
        if (closed.find(ancestor) != closed.end())
            continue;
        closed.insert(ancestor);
        FactPair ancestor_fact = get_fact(ancestor->get_landmark());
        groups[ancestor_fact.var].push_back(ancestor_fact.value);
        for (const auto &parent_and_edge : ancestor->parents) {
            const LandmarkNode *parent = parent_and_edge.first;
            open.push_back(parent);
        }
    }
    return groups;
}
}

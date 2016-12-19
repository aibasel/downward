#include "utils_landmarks.h"

#include "../option_parser.h"

#include "../landmarks/exploration.h"
#include "../landmarks/landmark_factory_h_m.h"
#include "../landmarks/landmark_graph.h"

#include "../utils/memory.h"

#include <algorithm>

using namespace std;
using namespace landmarks;

namespace cegar {
static FactPair get_fact(const LandmarkNode &node) {
    /* We assume that the given LandmarkNodes are from an h^m landmark
       graph with m=1. */
    assert(node.facts.size() == 1);
    return node.facts[0];
}

shared_ptr<LandmarkGraph> get_landmark_graph(const shared_ptr<AbstractTask> &task) {
    Options exploration_opts;
    exploration_opts.set<shared_ptr<AbstractTask>>("transform", task);
    exploration_opts.set<bool>("cache_estimates", false);
    Exploration exploration(exploration_opts);

    Options hm_opts;
    hm_opts.set<int>("m", 1);
    // h^m doesn't produce reasonable orders anyway.
    hm_opts.set<bool>("reasonable_orders", false);
    hm_opts.set<bool>("only_causal_landmarks", false);
    hm_opts.set<bool>("disjunctive_landmarks", false);
    hm_opts.set<bool>("conjunctive_landmarks", false);
    hm_opts.set<bool>("no_orders", false);
    hm_opts.set<int>("lm_cost_type", NORMAL);
    LandmarkFactoryHM lm_graph_factory(hm_opts);

    return lm_graph_factory.compute_lm_graph(task, exploration);
}

vector<FactPair> get_fact_landmarks(const LandmarkGraph &graph) {
    vector<FactPair> facts;
    const set<LandmarkNode *> &nodes = graph.get_nodes();
    for (const LandmarkNode *node : nodes) {
        facts.push_back(get_fact(*node));
    }
    sort(facts.begin(), facts.end());
    return facts;
}

VarToValues get_prev_landmarks(const LandmarkGraph &graph, const FactPair &fact) {
    VarToValues groups;
    LandmarkNode *node = graph.get_landmark(fact);
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
        FactPair ancestor_fact = get_fact(*ancestor);
        groups[ancestor_fact.var].push_back(ancestor_fact.value);
        for (const auto &parent_and_edge : ancestor->parents) {
            const LandmarkNode *parent = parent_and_edge.first;
            open.push_back(parent);
        }
    }
    return groups;
}
}

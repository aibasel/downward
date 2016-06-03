#include "utils_landmarks.h"

#include "../option_parser.h"

#include "../landmarks/h_m_landmarks.h"
#include "../landmarks/landmark_graph.h"

#include "../utils/memory.h"

#include <algorithm>

using namespace std;
using namespace landmarks;

namespace cegar {
static Fact get_fact(const LandmarkNode &node) {
    /* We assume that the given LandmarkNodes are from an h^m landmark
       graph with m=1. */
    assert(node.vars.size() == 1);
    assert(node.vals.size() == 1);
    return Fact(node.vars[0], node.vals[0]);
}

unique_ptr<LandmarkGraph> get_landmark_graph() {
    Options opts = Options();
    opts.set<int>("cost_type", NORMAL);
    opts.set<bool>("cache_estimates", false);
    opts.set<int>("m", 1);
    // h^m doesn't produce reasonable orders anyway.
    opts.set<bool>("reasonable_orders", false);
    opts.set<bool>("only_causal_landmarks", false);
    opts.set<bool>("disjunctive_landmarks", false);
    opts.set<bool>("conjunctive_landmarks", false);
    opts.set<bool>("no_orders", false);
    opts.set<int>("lm_cost_type", NORMAL);
    opts.set<bool>("supports_conditional_effects", false);
    /* This function assumes that the exploration object is not used
       after the landmark graph has been created. */
    Exploration exploration(opts);
    opts.set<Exploration *>("explor", &exploration);
    HMLandmarks lm_graph_factory(opts);
    unique_ptr<LandmarkGraph> landmark_graph(lm_graph_factory.compute_lm_graph());
    landmark_graph->invalidate_exploration_for_cegar();
    return landmark_graph;
}

vector<Fact> get_fact_landmarks(const LandmarkGraph &graph) {
    vector<Fact> facts;
    const set<LandmarkNode *> &nodes = graph.get_nodes();
    for (LandmarkNode *node : nodes) {
        facts.push_back(get_fact(*node));
    }
    sort(facts.begin(), facts.end());
    return facts;
}

VarToValues get_prev_landmarks(const LandmarkGraph &graph, const Fact &fact) {
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
        if (closed.count(ancestor) == 1)
            continue;
        closed.insert(ancestor);
        Fact ancestor_fact = get_fact(*ancestor);
        groups[ancestor_fact.var].push_back(ancestor_fact.value);
        for (const auto &parent_and_edge : ancestor->parents) {
            const LandmarkNode *parent = parent_and_edge.first;
            open.push_back(parent);
        }
    }
    return groups;
}
}

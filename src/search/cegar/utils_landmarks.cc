#include "utils_landmarks.h"

#include "../option_parser.h"

#include "../landmarks/h_m_landmarks.h"
#include "../landmarks/landmark_graph.h"

#include <algorithm>
#include <fstream>

using namespace std;

namespace cegar {
Fact get_fact(const LandmarkNode *node) {
    assert(node);
    assert(node->vars.size() == 1);
    int var = node->vars[0];
    int value = node->vals[0];
    return Fact(var, value);
}

vector<Fact> get_fact_landmarks(shared_ptr<LandmarkGraph> landmark_graph) {
    vector<Fact> facts;
    const set<LandmarkNode *> &nodes = landmark_graph->get_nodes();
    for (LandmarkNode *node : nodes) {
        facts.push_back(get_fact(node));
    }
    sort(facts.begin(), facts.end());
    return facts;
}

shared_ptr<LandmarkGraph> get_landmark_graph() {
    Options opts = Options();
    opts.set<int>("cost_type", 0);
    opts.set<int>("memory_padding", 75);
    opts.set<int>("m", 1);
    // h^m doesn't produce reasonable orders anyway.
    opts.set<bool>("reasonable_orders", false);
    opts.set<bool>("only_causal_landmarks", false);
    opts.set<bool>("disjunctive_landmarks", false);
    opts.set<bool>("conjunctive_landmarks", false);
    opts.set<bool>("no_orders", false);
    opts.set<int>("lm_cost_type", 0);
    opts.set<bool>("supports_conditional_effects", false);
    Exploration exploration(opts);
    opts.set<Exploration *>("explor", &exploration);
    HMLandmarks lm_graph_factory(opts);
    return shared_ptr<LandmarkGraph>(lm_graph_factory.compute_lm_graph());
}

/*
  Do a breadth-first search through the landmark graph ignoring
  duplicates. Start at the node for the given fact and collect for each
  variable the facts that have to be made true before the fact is made
  true for the first time.
*/
VariableToValues get_prev_landmarks(shared_ptr<LandmarkGraph> landmark_graph, Fact fact) {
    unordered_map<int, unordered_set<int> > groups;
    LandmarkNode *node = landmark_graph->get_landmark(fact);
    assert(node);
    vector<const LandmarkNode *> open;
    unordered_set<const LandmarkNode *> closed;
    for (const auto &parent_and_edge: node->parents) {
        const LandmarkNode *parent = parent_and_edge.first;
        open.push_back(parent);
    }
    while (!open.empty()) {
        const LandmarkNode *ancestor = open.back();
        open.pop_back();
        if (closed.count(ancestor))
            continue;
        closed.insert(ancestor);
        Fact ancestor_fact = get_fact(ancestor);
        groups[ancestor_fact.first].insert(ancestor_fact.second);
        for (auto it = ancestor->parents.begin(); it != ancestor->parents.end(); ++it) {
            const LandmarkNode *parent = it->first;
            open.push_back(parent);
        }
    }
    return groups;
}

string get_node_name(Fact fact) {
    stringstream out;
    out << g_fact_names[fact.first][fact.second] << " (" << fact.first << "=" << fact.second << ")";
    return out.str();
}

void write_landmark_graph(shared_ptr<LandmarkGraph> graph) {
    const set<LandmarkNode *> &nodes = graph->get_nodes();
    set<LandmarkNode *, LandmarkNodeComparer> nodes2(nodes.begin(), nodes.end());

    ofstream dotfile("landmark-graph.dot");
    if (!dotfile.is_open()) {
        cerr << "dot file for causal graph could not be opened" << endl;
        exit_with(EXIT_CRITICAL_ERROR);
    }
    dotfile << "digraph landmarkgraph {" << endl;


    for (const auto *node_p : nodes2) {
        Fact node_fact = get_fact(node_p);
        for (const auto &parent_pair : node_p->parents) {
            const LandmarkNode *parent_p = parent_pair.first;
            Fact parent_fact = get_fact(parent_p);
            dotfile << "  \"" << get_node_name(parent_fact) << "\" -> "
                    << "\"" << get_node_name(node_fact) << "\";" << endl;
            // Mark initial state facts green.
            if (g_initial_state()[parent_fact.first] == parent_fact.second)
                dotfile << "  \"" << get_node_name(parent_fact) << "\" [color=green];" << endl;
            if (g_initial_state()[node_fact.first] == node_fact.second)
                dotfile << "  \"" << get_node_name(node_fact) << "\" [color=green];" << endl;
        }
    }
    // Mark goal facts red.
    for (size_t i = 0; i < g_goal.size(); i++) {
        dotfile << "  \"" << get_node_name(g_goal[i]) << "\" [color=red];" << endl;
    }
    dotfile << "}" << endl;
    dotfile.close();
}
}

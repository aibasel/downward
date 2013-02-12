#include "legacy_causal_graph.h"

#include "globals.h"

#include <algorithm>
#include <cassert>
#include <iostream>
using namespace std;

LegacyCausalGraph::LegacyCausalGraph(istream &in) {
    check_magic(in, "begin_CG");
    int var_count = g_variable_domain.size();
    arcs.resize(var_count);
    inverse_arcs.resize(var_count);
    edges.resize(var_count);
    for (int from_node = 0; from_node < var_count; from_node++) {
        int num_succ;
        in >> num_succ;
        arcs[from_node].reserve(num_succ);
        for (int j = 0; j < num_succ; j++) {
            // weight not needed so far
            int to_node, weight;
            in >> to_node;
            in >> weight;
            arcs[from_node].push_back(to_node);
            inverse_arcs[to_node].push_back(from_node);
            edges[from_node].push_back(to_node);
            edges[to_node].push_back(from_node);
        }
    }
    check_magic(in, "end_CG");

    for (int i = 0; i < var_count; i++) {
        sort(edges[i].begin(), edges[i].end());
        edges[i].erase(unique(edges[i].begin(), edges[i].end()), edges[i].end());
    }
}

const vector<int> &LegacyCausalGraph::get_successors(int var) const {
    return arcs[var];
}

const vector<int> &LegacyCausalGraph::get_predecessors(int var) const {
    return inverse_arcs[var];
}

const vector<int> &LegacyCausalGraph::get_neighbours(int var) const {
    return edges[var];
}

void LegacyCausalGraph::dump() const {
    cout << "Causal graph: " << endl;
    for (int i = 0; i < arcs.size(); i++) {
        cout << "dependent on var " << g_variable_name[i] << ": " << endl;
        for (int j = 0; j < arcs[i].size(); j++)
            cout << "  " << g_variable_name[arcs[i][j]] << ",";
        cout << endl;
    }
}

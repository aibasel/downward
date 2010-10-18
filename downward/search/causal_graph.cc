#include "causal_graph.h"
#include "globals.h"

#include <iostream>
#include <cassert>
using namespace std;

CausalGraph::CausalGraph(istream &in) {
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
    compute_ancestors();
}

const vector<int> &CausalGraph::get_successors(int var) const {
    return arcs[var];
}

const vector<int> &CausalGraph::get_predecessors(int var) const {
    return inverse_arcs[var];
}

const vector<int> &CausalGraph::get_neighbours(int var) const {
    return edges[var];
}

void CausalGraph::dump() const {
    cout << "Causal graph: " << endl;
    for (int i = 0; i < arcs.size(); i++) {
        cout << "dependent on var " << g_variable_name[i] << ": " << endl;
        for (int j = 0; j < arcs[i].size(); j++)
            cout << "  " << g_variable_name[arcs[i][j]] << ",";
        cout << endl;
    }
}

// TODO: put acyclicity in input
//bool CausalGraph::is_acyclic() const {
//  return acyclic;
//}


void CausalGraph::compute_ancestors() {
    int var_count = g_variable_domain.size();
    vector<bool> marked(var_count, false);
    ancestors.resize(var_count);
    for (int var_no = 0; var_no < var_count; var_no++) {
        vector<int> &var_ancestors = ancestors[var_no];
        compute_ancestors_dfs(var_no, marked, var_ancestors);
        ::sort(var_ancestors.begin(), var_ancestors.end());
        for (int i = 0; i < var_ancestors.size(); i++)
            marked[var_ancestors[i]] = false;
    }
}

void CausalGraph::compute_ancestors_dfs(
    int var_no, vector<bool> &marked, vector<int> &result) const {
    if (!marked[var_no]) {
        marked[var_no] = true;
        result.push_back(var_no);
        for (int i = 0; i < inverse_arcs[var_no].size(); i++)
            compute_ancestors_dfs(inverse_arcs[var_no][i], marked, result);
    }
}

bool CausalGraph::have_common_ancestor(int var1, int var2) const {
    // Algorithm to test if two sorted lists have a common element.
    // Proceeds similar to merge step in mergesort.
    vector<int>::const_iterator iter1 = ancestors[var1].begin();
    vector<int>::const_iterator end1 = ancestors[var1].end();
    vector<int>::const_iterator iter2 = ancestors[var2].begin();
    vector<int>::const_iterator end2 = ancestors[var2].end();
    while (iter1 != end1 && iter2 != end2) {
        if (*iter1 == *iter2) {
            return true;
        } else if (*iter1 < *iter2) {
            assert(iter1 + 1 == end1 || iter1[1] >= iter1[0]);
            ++iter1;
        } else {
            assert(iter2 + 1 == end2 || iter2[1] >= iter2[0]);
            ++iter2;
        }
    }
    return false;
}

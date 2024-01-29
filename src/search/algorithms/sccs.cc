#include "sccs.h"

#include <algorithm>

using namespace std;

namespace sccs {
static void dfs(
    const vector<vector<int>> &graph,
    int vertex,
    vector<int> &dfs_numbers,
    vector<int> &dfs_minima,
    vector<int> &stack_indices,
    vector<int> &stack,
    int &current_dfs_number,
    vector<vector<int>> &sccs) {
    int vertex_dfs_number = current_dfs_number++;
    dfs_numbers[vertex] = dfs_minima[vertex] = vertex_dfs_number;
    stack_indices[vertex] = stack.size();
    stack.push_back(vertex);

    const vector<int> &successors = graph[vertex];
    for (size_t i = 0; i < successors.size(); i++) {
        int succ = successors[i];
        int succ_dfs_number = dfs_numbers[succ];
        if (succ_dfs_number == -1) {
            dfs(graph, succ, dfs_numbers, dfs_minima, stack_indices, stack, current_dfs_number, sccs);
            dfs_minima[vertex] = min(dfs_minima[vertex], dfs_minima[succ]);
        } else if (succ_dfs_number < vertex_dfs_number && stack_indices[succ] != -1) {
            dfs_minima[vertex] = min(dfs_minima[vertex], succ_dfs_number);
        }
    }

    if (dfs_minima[vertex] == vertex_dfs_number) {
        int stack_index = stack_indices[vertex];
        vector<int> scc;
        for (size_t i = stack_index; i < stack.size(); i++) {
            scc.push_back(stack[i]);
            stack_indices[stack[i]] = -1;
        }
        stack.erase(stack.begin() + stack_index, stack.end());
        sccs.push_back(scc);
    }
}

vector<vector<int>> compute_maximal_sccs(
    const vector<vector<int>> &graph) {
    int node_count = graph.size();
    vector<int> dfs_numbers(node_count, -1);
    vector<int> dfs_minima(node_count, -1);
    vector<int> stack_indices(node_count, -1);
    vector<int> stack;
    stack.reserve(node_count);
    int current_dfs_number = 0;

    vector<vector<int>> sccs;
    for (int i = 0; i < node_count; i++) {
        if (dfs_numbers[i] == -1) {
            dfs(graph, i, dfs_numbers, dfs_minima, stack_indices, stack, current_dfs_number, sccs);
        }
    }

    reverse(sccs.begin(), sccs.end());
    return sccs;
}
}

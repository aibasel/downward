#include "sccs.h"

#include <algorithm>

using namespace std;

namespace sccs {
SCCs::SCCs(const vector<vector<int>> &graph_)
    : graph(graph_) {
}

const vector<vector<int>> &SCCs::get_result() {
    int node_count = graph.size();
    dfs_numbers.resize(node_count, -1);
    dfs_minima.resize(node_count, -1);
    stack_indices.resize(node_count, -1);
    stack.reserve(node_count);
    current_dfs_number = 0;

    for (int i = 0; i < node_count; i++)
        if (dfs_numbers[i] == -1)
            dfs(i);

    reverse(sccs.begin(), sccs.end());
    return sccs;
}

void SCCs::dfs(int vertex) {
    int vertex_dfs_number = current_dfs_number++;
    dfs_numbers[vertex] = dfs_minima[vertex] = vertex_dfs_number;
    stack_indices[vertex] = stack.size();
    stack.push_back(vertex);

    const vector<int> &successors = graph[vertex];
    for (size_t i = 0; i < successors.size(); i++) {
        int succ = successors[i];
        int succ_dfs_number = dfs_numbers[succ];
        if (succ_dfs_number == -1) {
            dfs(succ);
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
}

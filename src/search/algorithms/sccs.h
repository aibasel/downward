#ifndef ALGORITHMS_SCCS_H
#define ALGORITHMS_SCCS_H

#include <vector>

namespace sccs {
void dfs(
    const std::vector<std::vector<int>> &graph,
    int vertex,
    std::vector<int> &dfs_numbers,
    std::vector<int> &dfs_minima,
    std::vector<int> &stack_indices,
    std::vector<int> &stack,
    int &current_dfs_number,
    std::vector<std::vector<int>> &sccs);

/*
  This function implements Tarjan's linear-time algorithm for finding the
  maximal strongly connected components. It takes time proportional to the sum
  of the number of vertices and arcs.

  Parameter: a graph represented as a vector of vectors, where graph[i] is the
  vector of successors of vertex i.

  Return value: a vector of strongly connected components, each of which is a
  vector of vertices (ints). This is a partitioning of all vertices where each
  SCC is a maximal subset such that each node in an SCC is reachable from all
  other nodes in the SCC. Note that the derived graph where each SCC is a
  single "supernode" is necessarily acyclic. The SCCs returned by this function
  are in a topological sort order with regard to this derived DAG.
*/
std::vector<std::vector<int>> compute_maximal_sccs(
    const std::vector<std::vector<int>> &graph);
}
#endif

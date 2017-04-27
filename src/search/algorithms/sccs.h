#ifndef ALGORITHMS_SCCS_H
#define ALGORITHMS_SCCS_H

#include <vector>

namespace sccs {
/*
  This function implements Tarjan's algorithm for finding the strongly
  connected components of a directed graph. The runtime is O(n + m) for a
  directed graph with n vertices and m arcs.

  Input: a directed graph represented as a vector of vectors, where graph[i] is
  the vector of successors of vertex i.

  Output: a vector of strongly connected components, each of which is a vector
  of vertices (ints). This is a partitioning of all vertices where each SCC is
  a maximal subset such that each vertex in an SCC is reachable from all other
  vertexs in the SCC. Note that the derived graph where each SCC is a single
  "supervertex" is necessarily acyclic. The SCCs returned by this function are
  in a topological sort order with regard to this derived DAG.
*/
std::vector<std::vector<int>> compute_maximal_sccs(
    const std::vector<std::vector<int>> &graph);
}
#endif

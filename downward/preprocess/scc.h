#ifndef SCC_H
#define SCC_H

/*
  This implements Tarjan's linear-time algorithm for finding the maximal
  strongly connected components. It takes time proportional to the sum
  of the number of vertices and arcs.

  Instantiate class SCC with a graph represented as a vector of vectors,
  where graph[i] is the vector of successors of vertex i.

  Method get_result() returns a vector of strongly connected components,
  each of which is a vector of vertices (ints).
  This is a partitioning of all vertices where each SCC is a maximal subset
  such that each node in an SCC is reachable from all other nodes in the SCC.
  Note that the derived graph where each SCC is a single "supernode" is
  necessarily acyclic. The SCCs returned by get_result() are in a topological
  sort order with regard to this derived DAG.
*/

#include <vector>
using namespace std;

class SCC {
    const vector<vector<int> > &graph;

    // The following three are indexed by vertex number.
    vector<int> dfs_numbers;
    vector<int> dfs_minima;
    vector<int> stack_indices;

    vector<int> stack; // This is indexed by the level of recursion.
    vector<vector<int> > sccs;

    int current_dfs_number;

    void dfs(int vertex);
public:
    SCC(const vector<vector<int> > &theGraph) : graph(theGraph) {}
    vector<vector<int> > get_result();
};
#endif

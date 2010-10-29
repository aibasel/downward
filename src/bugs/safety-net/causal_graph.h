#ifndef CAUSAL_GRAPH_H
#define CAUSAL_GRAPH_H

#include <vector>
#include <map>
using namespace std;


class CausalGraph {

  //typedef map<Variable *, int> WeightedSuccessors;
  //typedef map<Variable *, WeightedSuccessors> WeightedGraph;
  //WeightedGraph weighted_graph;
  //bool acyclic;
  vector<vector<int> > arcs;
  vector<vector<int> > edges;
public:
  CausalGraph(istream &in);
  ~CausalGraph() {}
  const vector<int> &get_successors(int var) const;
  const vector<int> &get_neighbours(int var) const;
  void dump() const;
  //bool is_acyclic() const;
};

#endif

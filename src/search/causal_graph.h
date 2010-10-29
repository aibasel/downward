#ifndef CAUSAL_GRAPH_H
#define CAUSAL_GRAPH_H

#include <algorithm>
#include <iosfwd>
#include <map>
#include <vector>
using namespace std;


class CausalGraph {
    vector<vector<int> > arcs;
    vector<vector<int> > inverse_arcs;
    vector<vector<int> > edges;
public:
    CausalGraph(istream &in);
    ~CausalGraph() {}
    const vector<int> &get_successors(int var) const;
    const vector<int> &get_predecessors(int var) const;
    const vector<int> &get_neighbours(int var) const;
    void dump() const;
};

#endif

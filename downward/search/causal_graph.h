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
    vector<vector<int> > ancestors;

    void compute_ancestors();
    void compute_ancestors_dfs(int var_no, vector<bool> &marked,
                               vector<int> &result) const;
public:
    CausalGraph(istream &in);
    ~CausalGraph() {}
    const vector<int> &get_successors(int var) const;
    const vector<int> &get_predecessors(int var) const;
    const vector<int> &get_neighbours(int var) const;
    bool have_common_ancestor(int var1, int var2) const;
    void dump() const;
};

#endif

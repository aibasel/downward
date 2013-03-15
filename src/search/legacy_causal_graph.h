#ifndef LEGACY_CAUSAL_GRAPH_H
#define LEGACY_CAUSAL_GRAPH_H

#include <iosfwd>
#include <vector>


class LegacyCausalGraph {
    /*
      This version of the causal graph only considers pre->eff arcs,
      not eff--eff edges. This is not what current literature usually
      considers the causal graph, and hence we call it the "legacy"
      causal graph as a warning to implementers. The aim is to
      eventually get rid of it.
    */
    std::vector<std::vector<int> > arcs;
    std::vector<std::vector<int> > inverse_arcs;
    std::vector<std::vector<int> > edges;
public:
    LegacyCausalGraph(std::istream &in);
    ~LegacyCausalGraph() {}
    const std::vector<int> &get_successors(int var) const;
    const std::vector<int> &get_predecessors(int var) const;
    const std::vector<int> &get_neighbours(int var) const;
    void dump() const;
};

#endif

#ifndef MAX_CLIQUES_H
#define MAX_CLIQUES_H

#include <vector>

// Implementation of the Max Cliques algorithm by Tomita et al.
extern void compute_max_cliques(
    const std::vector<std::vector<int> > &graph,
    std::vector<std::vector<int> > &max_cliques);

#endif

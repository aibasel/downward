#ifndef PLANNING_UTILITIES_H
#define PLANNING_UTILITIES_H

#include <vector>

// Implementation of the Max Cliques algorithm by Tomita et al
int get_maximizing_vertex(const std::vector<int> &subg, const std::vector<int> &cand, const std::vector<std::vector<int> > &cgraph);
void expand(std::vector<int> &subg, std::vector<int> &cand, std::vector<int> &q_clique,
            const std::vector<std::vector<int> > &cgraph, std::vector<std::vector<int> > &max_cliques);
extern void compute_max_cliques(const std::vector<std::vector<int> > &cgraph, std::vector<std::vector<int> > &max_cliques);

#endif

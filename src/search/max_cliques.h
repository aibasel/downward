#ifndef MAX_CLIQUES_H
#define MAX_CLIQUES_H

#include <vector>

/* Implementation of the Max Cliques algorithm by Tomita et al.
   In the paper the authors use a compressed output of the cliques, such that the
   algorithm is in O(3^{n/3}).
   This implementation is in O(n 3^{n/3}) because the cliques are outputed explicitely.
   For a better runtime it could be useful to use bitvectors instead of vectors.
 */
extern void compute_max_cliques(
    const std::vector<std::vector<int> > &graph,
    std::vector<std::vector<int> > &max_cliques);

#endif

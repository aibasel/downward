#ifndef ALGORITHMS_MAX_CLIQUES_H
#define ALGORITHMS_MAX_CLIQUES_H

#include <vector>

namespace max_cliques {
/* Implementation of the Max Cliques algorithm by Tomita et al. See:

   Etsuji Tomita, Akira Tanaka and Haruhisa Takahashi, The Worst-Case
   Time Complexity for Generating All Maximal Cliques. Proceedings of
   the 10th Annual International Conference on Computing and
   Combinatorics (COCOON 2004), pp. 161-170, 2004.

   In the paper the authors use a compressed output of the cliques,
   such that the algorithm is in O(3^{n/3}).

   This implementation is in O(n 3^{n/3}) because the cliques are
   output explicitly. For a better runtime it could be useful to
   use bit vectors instead of vectors.
 */
extern void compute_max_cliques(
    const std::vector<std::vector<int>> &graph,
    std::vector<std::vector<int>> &max_cliques);
}

#endif

#ifndef PATTERN_COLLECTION_H
#define PATTERN_COLLECTION_H

#include "pdb_heuristic.h"

#include <vector>

class PatternCollection {
    std::vector<std::vector<int> > pattern_collection;
    int number_patterns;
    void precompute_additive_vars();
    void compute_max_cliques();
    std::vector<std::vector<int> > cgraph; // compatibility graph for the pattern collection
    std::vector<std::vector<int> > max_cliques; // final computed max_cliques
    std::vector<std::vector<bool> > are_additive; // variables which are additive
    //bool are_additive(int pattern1, int pattern2) const;
    void build_cgraph();
    int get_maxi_vertex(const std::vector<int> &subg, const std::vector<int> &cand) const; // TODO find nicer name :)
    void max_cliques_expand(std::vector<int> &subg, std::vector<int> &cand, std::vector<int> &q_clique); // implements the CLIQUES-algorithmn from Tomita et al
public:
    explicit PatternCollection(const std::vector<std::vector<int> > &pat_coll);
    ~PatternCollection();
    std::vector<PDBAbstraction> pattern_databases; // final pattern databases
    int get_heuristic_value(const State &state) const; // returns the canonical heuristic value (with respect
        // to the pattern collection) for a state
    void dump() const;
};

#endif

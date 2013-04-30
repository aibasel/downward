#ifndef PDBS_CANONICAL_PDBS_HEURISTIC_H
#define PDBS_CANONICAL_PDBS_HEURISTIC_H

#include "../heuristic.h"

#include <vector>

// Implements the canonical heuristic function.
class PDBHeuristic;
class CanonicalPDBsHeuristic : public Heuristic {
    int size; // the sum of all abstract state sizes of all pdbs in the collection
    std::vector<std::vector<PDBHeuristic *> > max_cliques; // final computed max_cliques
    std::vector<std::vector<bool> > are_additive; // pair of variables which are additive
    std::vector<PDBHeuristic *> pattern_databases; // final pattern databases

    /* Returns true iff the two patterns are additive i.e. there is no operator
       which affects variables in pattern one as well as in pattern two. */
    bool are_patterns_additive(const std::vector<int> &patt1,
                               const std::vector<int> &patt2) const;

    /* Computation of maximal additive subsets of patterns. A maximal clique represents
       a maximal additive subset of patterns. */
    void compute_max_cliques();

    /* Precompution of pairwise additive variables i.e. variables where no operator affects
       both variables at the same time. */
    void compute_additive_vars();

    // does not recompute max_cliques
    void _add_pattern(const std::vector<int> &pattern);

    void dump_cgraph(const std::vector<std::vector<int> > &cgraph) const;
    void dump_cliques() const;
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    CanonicalPDBsHeuristic(const Options &opts);
    virtual ~CanonicalPDBsHeuristic();

    // add a new pattern to the collection and recomputes maximal cliques
    void add_pattern(const std::vector<int> &pattern);

    /* Prune pattern set P = {P_1, ..., P_k} if there exists another pattern set
       Q = {Q_1, ..., Q_l} where each P_i is a subset of some Q_j:
       this implies h^P <= h^Q for all states. */
    void dominance_pruning();

    // checks for all max cliques if they would be additive to this pattern
    void get_max_additive_subsets(const std::vector<int> &new_pattern,
                                  std::vector<std::vector<PDBHeuristic *> > &max_additive_subsets);
    const std::vector<PDBHeuristic *> &get_pattern_databases() const {return pattern_databases; }
    int get_size() const {return size; }
    void dump() const;
};

#endif

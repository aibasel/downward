#ifndef PDBS_CANONICAL_PDBS_HEURISTIC_H
#define PDBS_CANONICAL_PDBS_HEURISTIC_H

#include "../heuristic.h"

#include <vector>

class PatternDatabase;

// Implements the canonical heuristic function.
class CanonicalPDBsHeuristic : public Heuristic {
    // The sum of all abstract state sizes of all pdbs in the collection.
    int size;
    // A maximal clique represents a maximal additive subset of patterns.
    std::vector<std::vector<PatternDatabase *> > max_cliques;
    // A pair of variables is additive if no operatorhas an effect on both.
    std::vector<std::vector<bool> > are_additive;
    std::vector<PatternDatabase *> pattern_databases;

    /* Returns true iff the two patterns are additive i.e. there is no operator
       which affects variables in pattern one as well as in pattern two. */
    bool are_patterns_additive(const std::vector<int> &pattern1,
                               const std::vector<int> &pattern2) const;

    // Precomputes maximal additive subsets of patterns.
    void compute_max_cliques();

    /* Precomputes pairwise additive variables i.e. variables where
       no operator affects both variables at the same time. */
    void compute_additive_vars();

    // Adds a PDB for pattern but does not recompute max_cliques.
    void add_pdb_for_pattern(const std::vector<int> &pattern);

    void dump_cgraph(const std::vector<std::vector<int> > &cgraph) const;
    void dump_cliques() const;
protected:
    virtual void initialize() override;
    virtual int compute_heuristic(const GlobalState &state) override;
public:
    explicit CanonicalPDBsHeuristic(const Options &opts);
    virtual ~CanonicalPDBsHeuristic();

    /* TODO: we want to get rid of compute_heuristic(const GlobalState &state)
       and change the interface to only use State objects. While we are doing
       this, the following method already allows to get the heuristic value
       for a State object. */
    int compute_heuristic(const State &state) const;

    // Adds a new pattern to the collection and recomputes maximal cliques.
    void add_pattern(const std::vector<int> &pattern);

    /* Prunes pattern set P = {P_1, ..., P_k} if there exists another
       pattern set Q = {Q_1, ..., Q_l} where each P_i is a subset of some Q_j:
       this implies h^P <= h^Q for all states. */
    void dominance_pruning();

    // checks for all max cliques if they would be additive to this pattern
    void get_max_additive_subsets(
        const std::vector<int> &new_pattern,
        std::vector<std::vector<PatternDatabase *> > &max_additive_subsets);

    /*
      The following method offers a quick dead-end check for the
      sampling procedure of iPDB. This exists because we can much more
      efficiently test if the canonical heuristic is infinite than
      computing the exact heuristic value.
    */
    bool is_dead_end(const State &state) const;
    const std::vector<PatternDatabase *> &get_pattern_databases() const {
        return pattern_databases;
    }
    int get_size() const {
        return size;
    }
    void dump() const;
};

#endif

#ifndef PDBS_INCREMENTAL_CANONICAL_PDBS_H
#define PDBS_INCREMENTAL_CANONICAL_PDBS_H

#include "types.h"

#include "../task_proxy.h"

#include <memory>
#include <vector>

class IncrementalCanonicalPDBs {
    const std::shared_ptr<AbstractTask> task;
    TaskProxy task_proxy;

    // The sum of all abstract state sizes of all pdbs in the collection.
    int size;
    // A maximal clique represents a maximal additive subset of patterns.
    std::shared_ptr<PDBCliques> max_cliques;
    // A pair of variables is additive if no operatorhas an effect on both.
    std::vector<std::vector<bool>> are_additive;
    std::shared_ptr<PDBCollection> pattern_databases;

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

public:
    // TODO issue585: the old code supported heuristic caching. Do we need this?
    explicit IncrementalCanonicalPDBs(const std::shared_ptr<AbstractTask> task,
                                      const Patterns &intitial_patterns);
    virtual ~IncrementalCanonicalPDBs() = default;

    /* TODO issue585: factor out the common code with CanonicalPDBsHeuristic. */
    int compute_heuristic(const State &state) const;

    // Adds a new pattern to the collection and recomputes maximal cliques.
    void add_pattern(const std::vector<int> &pattern);
    // checks for all max cliques if they would be additive to this pattern
    void get_max_additive_subsets(
        const Pattern &new_pattern,
        PDBCliques &max_additive_subsets);

    /*
      The following method offers a quick dead-end check for the
      sampling procedure of iPDB. This exists because we can much more
      efficiently test if the canonical heuristic is infinite than
      computing the exact heuristic value.
    */
    bool is_dead_end(const State &state) const;

    std::shared_ptr<PDBCollection> get_pattern_databases() const {
        return pattern_databases;
    }

    std::shared_ptr<PDBCliques> get_cliques() {
        return max_cliques;
    }

    int get_size() const {
        return size;
    }
};

#endif

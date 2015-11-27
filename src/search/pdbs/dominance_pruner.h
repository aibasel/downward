#ifndef PDBS_DOMINANCE_PRUNER_H
#define PDBS_DOMINANCE_PRUNER_H

#include "types.h"

#include "../utilities.h"
#include "../utilities_hash.h"

#include <unordered_set>
#include <utility>


class DominancePruner {
    using PDBRelation = std::unordered_set<
              std::pair<PatternDatabase *, PatternDatabase *>>;

    PDBCollection &pattern_databases;
    PDBCliques &max_cliques;

    // Precomputed superset relation of patterns.
    PDBRelation superset_relation;
    void compute_superset_relation();

    void replace_pdb(std::shared_ptr<PatternDatabase> old_pdb,
                     std::shared_ptr<PatternDatabase> new_pdb);
    /*
      Collection superset dominates collection subset iff for every pattern
      p_subset in subset there is a pattern p_superset in superset where
      p_superset is a superset of p_subset.
    */
    bool clique_dominates(const PDBCollection &superset,
                          const PDBCollection &subset);

public:
    DominancePruner(PDBCollection &pattern_databases, PDBCliques &max_cliques);
    void prune();
};

#endif

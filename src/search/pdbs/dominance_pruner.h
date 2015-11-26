#ifndef PDBS_DOMINANCE_PRUNER_H
#define PDBS_DOMINANCE_PRUNER_H

#include "pattern_database.h"
#include "types.h"

#include "../utilities.h"
#include "../utilities_hash.h"

#include <unordered_set>
#include <vector>

class DominancePruner {
private:
    PDBCollection &pattern_databases;
    PDBCliques &max_cliques;

    // Precomputed superset relation of patterns.
    using PDBRelation = std::unordered_set<
              std::pair<PatternDatabase *, PatternDatabase *>>;
    PDBRelation superset_relation;
    void compute_superset_relation();

    void replace_pdb(std::shared_ptr<PatternDatabase> old_pdb,
                     std::shared_ptr<PatternDatabase> new_pdb);
    bool clique_dominates(const PDBCollection &c1, const PDBCollection &c2);

public:
    DominancePruner(PDBCollection &pattern_databases_, PDBCliques &max_cliques_);
    void prune();
};

#endif

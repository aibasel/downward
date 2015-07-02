#ifndef PDBS_DOMINANCE_PRUNER_H
#define PDBS_DOMINANCE_PRUNER_H

#include "pattern_database.h"

#include "../utilities.h"
#include "../utilities_hash.h"

#include <unordered_set>
#include <vector>

class DominancePruner {
private:
    std::vector<PatternDatabase *> &pattern_databases;
    std::vector<std::vector<PatternDatabase *> > &max_cliques;

    // Precomputed superset relation of patterns.
    typedef std::unordered_set<
        std::pair<PatternDatabase *, PatternDatabase *> > PDBRelation;
    PDBRelation superset_relation;
    void compute_superset_relation();

    void replace_pdb(PatternDatabase *old_pdb, PatternDatabase *new_pdb);
    bool clique_dominates(const std::vector<PatternDatabase *> &c1,
                          const std::vector<PatternDatabase *> &c2);

public:
    DominancePruner(std::vector<PatternDatabase *> &pattern_databases_,
                    std::vector<std::vector<PatternDatabase *> > &max_cliques_);
    void prune();
};

#endif

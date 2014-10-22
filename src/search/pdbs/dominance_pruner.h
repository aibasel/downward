#ifndef PDBS_DOMINANCE_PRUNER_H
#define PDBS_DOMINANCE_PRUNER_H

#include "pdb_heuristic.h"

#include "../utilities.h"

#include <vector>
#include <ext/hash_set>

class DominancePruner {
private:
    std::vector<PDBHeuristic *> &pattern_databases;
    std::vector<std::vector<PDBHeuristic *> > &max_cliques;

    // Precomputed superset relation of patterns.
    typedef __gnu_cxx::hash_set<std::pair<PDBHeuristic *, PDBHeuristic *>,
                                hash_pointer_pair> PDBRelation;
    PDBRelation superset_relation;
    void compute_superset_relation();

    void replace_pdb(PDBHeuristic *old_pdb, PDBHeuristic *new_pdb);
    bool clique_dominates(const std::vector<PDBHeuristic *> &c1,
                          const std::vector<PDBHeuristic *> &c2);

public:
    DominancePruner(std::vector<PDBHeuristic *> &pattern_databases_,
                    std::vector<std::vector<PDBHeuristic *> > &max_cliques_);
    void prune();
};

#endif

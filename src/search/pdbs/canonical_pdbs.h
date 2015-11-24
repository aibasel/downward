#ifndef PDBS_CANONICAL_PDBS_H
#define PDBS_CANONICAL_PDBS_H

#include "types.h"

#include <memory>

class State;

class CanonicalPDBs {
    std::shared_ptr<PDBCollection> pattern_databases;

    // A maximal clique represents a maximal additive subset of patterns.
    std::shared_ptr<PDBCliques> max_cliques;

public:
    CanonicalPDBs(std::shared_ptr<PDBCollection> pattern_databases,
                  std::shared_ptr<PDBCliques> max_cliques);
    ~CanonicalPDBs() = default;

    int get_value(const State &state) const;

    /* Prunes pattern set P = {P_1, ..., P_k} if there exists another
       pattern set Q = {Q_1, ..., Q_l} where each P_i is a subset of some Q_j:
       this implies h^P <= h^Q for all states. */
    void dominance_pruning();
};

#endif

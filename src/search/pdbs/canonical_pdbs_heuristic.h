#ifndef PDBS_CANONICAL_PDBS_HEURISTIC_H
#define PDBS_CANONICAL_PDBS_HEURISTIC_H

#include "types.h"

#include "../heuristic.h"

#include <memory>
#include <vector>

class PatternDatabase;

// Implements the canonical heuristic function.
class CanonicalPDBsHeuristic : public Heuristic {
    std::shared_ptr<PDBCollection> pattern_databases;

    // A maximal clique represents a maximal additive subset of patterns.
    std::shared_ptr<PDBCliques> max_cliques;

protected:
    virtual int compute_heuristic(const GlobalState &state) override;

public:
    explicit CanonicalPDBsHeuristic(const Options &opts);
    virtual ~CanonicalPDBsHeuristic() = default;

    /* TODO: we want to get rid of compute_heuristic(const GlobalState &state)
       and change the interface to only use State objects. While we are doing
       this, the following method already allows to get the heuristic value
       for a State object. */
    int compute_heuristic(const State &state) const;

    /* Prunes pattern set P = {P_1, ..., P_k} if there exists another
       pattern set Q = {Q_1, ..., Q_l} where each P_i is a subset of some Q_j:
       this implies h^P <= h^Q for all states. */
    void dominance_pruning();
};

#endif

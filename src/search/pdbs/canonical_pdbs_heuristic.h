#ifndef PDBS_CANONICAL_PDBS_HEURISTIC_H
#define PDBS_CANONICAL_PDBS_HEURISTIC_H

#include "canonical_pdbs.h"
#include "types.h"

#include "../heuristic.h"

#include <memory>
#include <vector>

class PatternDatabase;

// Implements the canonical heuristic function.
class CanonicalPDBsHeuristic : public Heuristic {
    CanonicalPDBs canonical_pdbs;

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
};

#endif

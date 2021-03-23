#ifndef PDBS_CANONICAL_PDBS_HEURISTIC_H
#define PDBS_CANONICAL_PDBS_HEURISTIC_H

#include "canonical_pdbs.h"

#include "../heuristic.h"

namespace options {
class OptionParser;
}

namespace pdbs {
// Implements the canonical heuristic function.
class CanonicalPDBsHeuristic : public Heuristic {
    CanonicalPDBs canonical_pdbs;

protected:
    virtual int compute_heuristic(const State &ancestor_state) override;

public:
    explicit CanonicalPDBsHeuristic(const options::Options &opts);
    virtual ~CanonicalPDBsHeuristic() = default;
};

void add_canonical_pdbs_options_to_parser(options::OptionParser &parser);
}

#endif

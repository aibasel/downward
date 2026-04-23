#ifndef PDBS_CANONICAL_PDBS_H
#define PDBS_CANONICAL_PDBS_H

#include "types.h"

#include <memory>

class State;

namespace pdbs {
class CanonicalPDBs {
    std::shared_ptr<PDBCollection> pdbs;
    std::shared_ptr<std::vector<PatternClique>> pattern_cliques;

public:
    CanonicalPDBs(
        const std::shared_ptr<PDBCollection> &pdbs,
        const std::shared_ptr<std::vector<PatternClique>> &pattern_cliques);
    ~CanonicalPDBs() = default;

    int get_value(const State &state) const;
};
}

#endif

#ifndef PDBS_CANONICAL_PDBS_H
#define PDBS_CANONICAL_PDBS_H

#include "types.h"

#include <memory>

class State;

namespace pdbs {
class CanonicalPDBs {
    std::shared_ptr<MaxAdditivePDBSubsets> max_additive_subsets;

public:
    CanonicalPDBs(std::shared_ptr<PDBCollection> pattern_databases,
                  std::shared_ptr<MaxAdditivePDBSubsets> max_additive_subsets,
                  bool dominance_pruning);
    ~CanonicalPDBs() = default;

    int get_value(const State &state) const;
};
}

#endif

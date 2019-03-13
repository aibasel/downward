#ifndef PDBS_CANONICAL_PDBS_H
#define PDBS_CANONICAL_PDBS_H

#include "types.h"

#include <memory>

class State;

namespace pdbs {
class CanonicalPDBs {
    std::shared_ptr<PDBCollection> pdbs;
    std::shared_ptr<MaxAdditivePDBSubsets> max_additive_subsets;
    // Used to avoid allocating memory in each call to get_value.
    mutable std::vector<int> h_values;

public:
    CanonicalPDBs(
        const std::shared_ptr<PDBCollection> &pdbs,
        const std::shared_ptr<MaxAdditivePDBSubsets> &max_additive_subsets);
    ~CanonicalPDBs() = default;

    int get_value(const State &state) const;
};
}

#endif

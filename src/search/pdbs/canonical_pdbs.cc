#include "canonical_pdbs.h"

#include "pattern_database.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>

using namespace std;

namespace pdbs {
CanonicalPDBs::CanonicalPDBs(
    const shared_ptr<MaxAdditivePDBSubsets> &max_additive_subsets_)
    : max_additive_subsets(max_additive_subsets_) {
    assert(max_additive_subsets);
}

int CanonicalPDBs::get_value(const State &state) const {
    // If we have an empty collection, then max_additive_subsets = { \emptyset }.
    assert(!max_additive_subsets->empty());
    int max_h = 0;
    for (const auto &subset : *max_additive_subsets) {
        int subset_h = 0;
        for (const shared_ptr<PatternDatabase> &pdb : subset) {
            /* Experiments showed that it is faster to recompute the
               h values than to cache them in an unordered_map. */
            int h = pdb->get_value(state);
            if (h == numeric_limits<int>::max())
                return numeric_limits<int>::max();
            subset_h += h;
        }
        max_h = max(max_h, subset_h);
    }
    return max_h;
}
}

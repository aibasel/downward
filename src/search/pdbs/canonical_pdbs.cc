#include "canonical_pdbs.h"

#include "pattern_database.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>

using namespace std;

namespace pdbs {
CanonicalPDBs::CanonicalPDBs(
    const shared_ptr<PDBCollection> &pdbs,
    const shared_ptr<MaxAdditivePDBSubsets> &max_additive_subsets)
    : pdbs(pdbs), max_additive_subsets(max_additive_subsets) {
    assert(pdbs);
    assert(max_additive_subsets);
    h_values.reserve(pdbs->size());
}

int CanonicalPDBs::get_value(const State &state) const {
    // If we have an empty collection, then max_additive_subsets = { \emptyset }.
    assert(!max_additive_subsets->empty());
    int max_h = 0;
    h_values.clear();
    for (const shared_ptr<PatternDatabase> &pdb : *pdbs) {
        int h_value = pdb->get_value(state);
        if (h_value == numeric_limits<int>::max()) {
            return numeric_limits<int>::max();
        }
        h_values.push_back(h_value);
    }
    for (const vector<int> &subset : *max_additive_subsets) {
        int subset_h = 0;
        for (int pdb_index : subset) {
            subset_h += h_values[pdb_index];
        }
        max_h = max(max_h, subset_h);
    }
    return max_h;
}
}

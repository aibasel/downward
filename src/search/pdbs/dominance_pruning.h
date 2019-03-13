#ifndef PDBS_DOMINANCE_PRUNING_H
#define PDBS_DOMINANCE_PRUNING_H

#include "types.h"

namespace pdbs {
/*
  Collection superset dominates collection subset iff for every pattern
  p_subset in subset there is a pattern p_superset in superset where
  p_superset is a superset of p_subset.
*/
extern void prune_dominated_subsets(
    PatternCollection &patterns,
    PDBCollection &pdbs,
    MaxAdditivePDBSubsets &max_additive_subsets,
    int num_variables,
    double max_time);
}

#endif

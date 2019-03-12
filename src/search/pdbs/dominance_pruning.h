#ifndef PDBS_DOMINANCE_PRUNING_H
#define PDBS_DOMINANCE_PRUNING_H

#include <memory>

namespace pdbs {
class PatternCollectionInformation;

/*
  Collection superset dominates collection subset iff for every pattern
  p_subset in subset there is a pattern p_superset in superset where
  p_superset is a superset of p_subset.
*/
extern PatternCollectionInformation prune_dominated_subsets(
    PatternCollectionInformation &pci,
    int num_variables,
    double max_time);
}

#endif

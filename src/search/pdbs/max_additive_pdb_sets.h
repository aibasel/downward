#ifndef PDBS_MAX_ADDITIVE_PDB_SETS_H
#define PDBS_MAX_ADDITIVE_PDB_SETS_H

#include "types.h"

#include <memory>
#include <vector>

class TaskProxy;

namespace pdbs {
using VariableAdditivity = std::vector<std::vector<bool>>;

extern VariableAdditivity compute_additive_vars(const TaskProxy &task_proxy);

/* Returns true iff the two patterns are additive i.e. there is no operator
   which affects variables in pattern one as well as in pattern two. */
extern bool are_patterns_additive(const Pattern &pattern1,
                                  const Pattern &pattern2,
                                  const VariableAdditivity &are_additive);

/*
  Computes maximal additive subsets of patterns.
*/
extern std::shared_ptr<MaxAdditivePDBSubsets> compute_max_additive_subsets(
    const PDBCollection &pdbs, const VariableAdditivity &are_additive);

/*
  We compute additive pattern sets S with the property that we could
  add the new pattern P to S and still have an additive pattern set.

  Ideally, we would like to return all *maximal* sets S with this
  property (w.r.t. set inclusion), but we don't currently
  guarantee this. (What we guarantee is that all maximal such sets
  are *included* in the result, but the result could contain
  duplicates or sets that are subsets of other sets in the
  result.)

  We currently implement this as follows:

  * Consider all maximal additive subsets of the current collection.
  * For each additive subset S, take the subset S' that contains
    those patterns that are additive with the new pattern P.
  * Include the subset S' in the result.

  As an optimization, we actually only include S' in the result if
  it is non-empty. However, this is wrong if *all* subsets we get
  are empty, so we correct for this case at the end.

  This may include dominated elements and duplicates in the result.
  To avoid this, we could instead use the following algorithm:

  * Let N (= neighbours) be the set of patterns in our current
    collection that are additive with the new pattern P.
  * Let G_N be the compatibility graph of the current collection
    restricted to set N (i.e. drop all non-neighbours and their
    incident edges.)
  * Return the maximal cliques of G_N.

  One nice thing about this alternative algorithm is that we could
  also use it to incrementally compute the new set of maximal additive
  pattern sets after adding the new pattern P:

  G_N_cliques = max_cliques(G_N)   // as above
  new_max_cliques = (old_max_cliques \setminus G_N_cliques) \union
                    { clique \union {P} | clique in G_N_cliques}

  That is, the new set of maximal cliques is exactly the set of
  those "old" cliques that we cannot extend by P
  (old_max_cliques \setminus G_N_cliques) and all
  "new" cliques including P.
  */
extern MaxAdditivePDBSubsets compute_max_additive_subsets_with_pattern(
    const MaxAdditivePDBSubsets &known_additive_subsets,
    const Pattern &new_pattern,
    const VariableAdditivity &are_additive);
}

#endif

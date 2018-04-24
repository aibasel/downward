#ifndef MERGE_AND_SHRINK_UTILS_H
#define MERGE_AND_SHRINK_UTILS_H

#include "types.h"

#include <memory>
#include <vector>

namespace merge_and_shrink {
class FactoredTransitionSystem;
class ShrinkStrategy;
class TransitionSystem;

/*
  Determine if any of the two factors at indices index1 and index2 must be
  shrunk according to the given size limits max_states* and
  shrink_treshold_before_merge: before merging, the factors may have at most
  max_states_before_merge states, and their product may hae at most
  max_states_after_merge states. If the size of a factor is below
  shrink_treshold_before_merge, shrinking is triggered with the current size
  of the factor as the target, to allow exploiting perfect shrink opportunities.

  If shrinking is triggered, apply the abstraction to the two factors
  within the factored transition system. Return true iff at least one of the
  factors was shrunk.
*/
extern bool shrink_before_merge_step(
    FactoredTransitionSystem &fts,
    int index1,
    int index2,
    int max_states,
    int max_states_before_merge,
    int shrink_threshold_before_merge,
    const ShrinkStrategy &shrink_strategy,
    Verbosity verbosity);

/*
  Prune unreachable and/or irrelevant states of the factor at index. This
  requires that init and/or goal distances have been computed accordingly.
  Return true iff any states have been pruned.

  TODO: maybe this functionality belongs to a new class PruneStrategy.
*/
extern bool prune_step(
    FactoredTransitionSystem &fts,
    int index,
    bool prune_unreachable_states,
    bool prune_irrelevant_states,
    Verbosity verbosity);

/*
  Compute the abstraction mapping based on the given state equivalence
  relation.
*/
extern std::vector<int> compute_abstraction_mapping(
    int num_states,
    const StateEquivalenceRelation &equivalence_relation);

extern bool is_goal_relevant(const TransitionSystem &ts);
}

#endif

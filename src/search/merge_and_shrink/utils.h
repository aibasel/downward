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
  shrunk according to the given size limits (max_states*), or if shrinking
  should be triggered nevertheless (shrink_treshold_before_merge). See
  compute_shrink_sizes for a detailed description of how target sizes are
  computed. If shrinking is triggered, apply the abstraction to the two factors
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

  TODO: maybe this functionality belongs to a new class PruningStrategy.
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

/*
  Copy the two transition systems at the given indices, possibly shrink them
  according to the same rules as merge-and-shrink does, and return their
  product.
*/
extern std::unique_ptr<TransitionSystem> shrink_before_merge_externally(
    const FactoredTransitionSystem &fts,
    int index1,
    int index2,
    const ShrinkStrategy &shrink_strategy,
    int max_states,
    int max_states_before_merge,
    int shrink_threshold_before_merge);
}

#endif

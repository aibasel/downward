#ifndef MERGE_AND_SHRINK_UTILS_H
#define MERGE_AND_SHRINK_UTILS_H

#include "types.h"

#include <vector>

namespace merge_and_shrink {
class FactoredTransitionSystem;
class ShrinkStrategy;
class TransitionSystem;

/*
  Compute target sizes for shrinking two transition systems with sizes size1
  and size2 before they are merged. Use the following rules:
  1) Right before merging, the transition systems may have at most
     max_states_before_merge states.
  2) Right after merging, the product has at most max_states_after_merge states.
  3) Transition systems are shrunk as little as necessary to satisfy the above
     constraints. (If possible, neither is shrunk at all.)
  There is often a Pareto frontier of solutions following these rules. In this
  case, balanced solutions (where the target sizes are close to each other)
  are preferred over less balanced ones.
*/
extern std::pair<int, int> compute_shrink_sizes(
    int size1,
    int size2,
    int max_states_before_merge,
    int max_states_after_merge);

/*
  This method checks if the transition system of the factor at index violates
  the size limit given via new_size (e.g. as computed by compute_shrink_sizes)
  or the threshold shrink_threshold_before_merge that triggers shrinking even
  if the size limit is not violated. If so, trigger the shrinking process.
  Return true iff the factor was actually shrunk.
*/
extern bool shrink_factor(
    FactoredTransitionSystem &fts,
    int index,
    int new_size,
    int shrink_threshold_before_merge,
    const ShrinkStrategy &shrink_strategy,
    Verbosity verbosity);

/*
  Prune unreachable and/or irrelevant states of the factor at index. This
  requires that init and/or goal distances have been computed accordingly.
  Return true iff any states have been pruned.

  TODO: maybe this functionality belongs to a new class PruningStrategy.
*/
extern bool prune_factor(
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

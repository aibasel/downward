#ifndef MERGE_AND_SHRINK_UTILS_H
#define MERGE_AND_SHRINK_UTILS_H

#include "types.h"

#include <memory>
#include <vector>

namespace utils {
class LogProxy;
}

namespace merge_and_shrink {
class FactoredTransitionSystem;
class ShrinkStrategy;
class TransitionSystem;

/*
  Compute target sizes for shrinking two transition systems with sizes size1
  and size2 before they are merged. Use the following rules:
  1) Right before merging, the transition systems may have at most
     max_states_before_merge states.
  2) Right after merging, the product may have most max_states_after_merge
     states.
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
  This function first determines if any of the two factors at indices index1
  and index2 must be shrunk according to the given size limits max_states and
  max_states_before_merge, using the function compute_shrink_sizes (see above).
  If not, then the function further checks if any of the two factors has a
  size larger than shrink_treshold_before_merge, in which case shrinking is
  still triggered.

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
    utils::LogProxy &log);

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
    utils::LogProxy &log);

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

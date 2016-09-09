#ifndef MERGE_AND_SHRINK_UTILS_H
#define MERGE_AND_SHRINK_UTILS_H

#include <memory>
#include <vector>

namespace merge_and_shrink {
class FactoredTransitionSystem;
class ShrinkStrategy;
enum class Verbosity;

/*
  Compute target sizes for shrinking two transition systems with sizes size1
  and size2 before they are merged. Use the following rules:
  1) Right before merging, the transition systems may have at most
     max_states_before_merge states.
  2) Right after merging, the product has at most max_states states.
  3) Transition systems shrunk as little as necessary to satisfy the above
     constraints. (If possible, neither is shrunk at all.)
  There is often a Pareto frontier of solutions following these rules. In this
  case, balanced solutions (where the target sizes are close to each other)
  are preferred over less balanced ones.
*/
extern std::pair<int, int> compute_shrink_sizes(
    int size1, int size2, int max_states, int max_states_before_merge);

/*
  This method checks if the transition system specified via index violates
  the size limit (given via new_size as computed by compute_shrink_sizes) or
  the threshold that triggers shrinking even if the size limit is not violated
  (shrink_threshold_before_merge). If so, the given shrink strategy
  (shrink_strategy) is used to reduce the size of the transition system to at
  most new_size.
*/
extern bool shrink_transition_system(
    FactoredTransitionSystem &fts,
    int index,
    int new_size,
    int shrink_threshold_before_merge,
    const std::shared_ptr<ShrinkStrategy> &shrink_strategy,
    Verbosity verbosity);

/*
  This method copies the entries given via ts_index1 and ts_index2, shrinks
  them according to the usual rules (i.e. by using compute_shrink_sizes and
  shrink_transition_system), and then merges them. The given factored
  transition system fts is extended with those three new entries. The caller
  is responsible to delete those copies afterwards with a call to
  fts.release_copies().
*/
extern int shrink_and_merge_temporarily(
    FactoredTransitionSystem &fts,
    int ts_index1,
    int ts_index2,
    const std::shared_ptr<ShrinkStrategy> &shrink_strategy,
    int max_states,
    int max_states_before_merge,
    int shrink_threshold_before_merge);
}

#endif

#ifndef MERGE_AND_SHRINK_UTILS_H
#define MERGE_AND_SHRINK_UTILS_H

#include <memory>
#include <vector>

namespace merge_and_shrink {
class FactoredTransitionSystem;
class ShrinkStrategy;
enum class Verbosity;

/*
  If any of size1 and size2 is larger than max_states_before_merge or
  if the product of size1 and size2 is larger than max_states, this method
  computes balanced sizes so that these limits are respected again.
*/
extern std::pair<int, int> compute_shrink_sizes(
    int size1, int size2, int max_states, int max_states_before_merge);

/*
  This method checks if the transition system specified via index violates
  the hard size limit (given via new_size) or the soft size limit
  (shrink_threshold_before_merge) and uses shrink_strategy to reduce
  the size of the transition system if necessary.
*/

extern bool shrink_transition_system(
    FactoredTransitionSystem &fts,
    int index,
    int new_size,
    int shrink_threshold_before_merge,
    const std::shared_ptr<ShrinkStrategy> &shrink_strategy,
    Verbosity verbosity);

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

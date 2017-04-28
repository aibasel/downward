#ifndef MERGE_AND_SHRINK_UTILS_H
#define MERGE_AND_SHRINK_UTILS_H

#include <vector>

namespace merge_and_shrink {
class FactoredTransitionSystem;
class ShrinkStrategy;
class TransitionSystem;
enum class Verbosity;

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

extern bool is_goal_relevant(const TransitionSystem &ts);
}

#endif

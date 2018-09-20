#ifndef MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_MIASM_UTILS_H
#define MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_MIASM_UTILS_H

#include <memory>

namespace merge_and_shrink {
class FactoredTransitionSystem;
class ShrinkStrategy;
class TransitionSystem;

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

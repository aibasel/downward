#ifndef MERGE_AND_SHRINK_MERGE_DFP_H
#define MERGE_AND_SHRINK_MERGE_DFP_H

#include "merge_strategy.h"

#include <vector>

namespace merge_and_shrink {
class TransitionSystem;

class MergeDFP : public MergeStrategy {
    // Store the "DFP" ordering in which transition systems should be considered.
    std::vector<int> transition_system_order;
    bool is_goal_relevant(const TransitionSystem &ts) const;
    std::vector<int> compute_label_ranks(int index) const;
    std::pair<int, int> compute_next_pair(
        const std::vector<int> &sorted_active_ts_indices) const;
public:
    MergeDFP(FactoredTransitionSystem &fts,
             std::vector<int> transition_system_order);
    virtual ~MergeDFP() override = default;
    virtual std::pair<int, int> get_next() override;
};
}

#endif

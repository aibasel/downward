#ifndef MERGE_AND_SHRINK_MERGE_DFP_H
#define MERGE_AND_SHRINK_MERGE_DFP_H

#include "merge_strategy.h"

namespace merge_and_shrink {
class TransitionSystem;

class MergeDFP : public MergeStrategy {
    // Store the "DFP" ordering in which transition systems should be considered.
    std::vector<int> transition_system_order;
    void compute_ts_order(const std::shared_ptr<AbstractTask> task);
    bool is_goal_relevant(const TransitionSystem &ts) const;
    void compute_label_ranks(const FactoredTransitionSystem &fts,
                             int index,
                             std::vector<int> &label_ranks) const;
    std::pair<int, int> compute_next_pair(
        const FactoredTransitionSystem &fts,
        const std::vector<int> &sorted_active_ts_indices) const;
protected:
    virtual void dump_strategy_specific_options() const override {}
public:
    MergeDFP();
    virtual ~MergeDFP() override = default;
    virtual void initialize(const std::shared_ptr<AbstractTask> task) override;

    virtual std::pair<int, int> get_next(FactoredTransitionSystem &fts) override;
    virtual std::string name() const override;
};
}

#endif

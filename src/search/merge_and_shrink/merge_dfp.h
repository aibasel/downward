#ifndef MERGE_AND_SHRINK_MERGE_DFP_H
#define MERGE_AND_SHRINK_MERGE_DFP_H

#include "merge_strategy.h"

class MergeDFP : public MergeStrategy {
    // border_atomic_composites is the first index at which a composite
    // transition system can be found in vector of all transition systems as passed
    // as argument to the get_next method.
    int border_atomics_composites;
    int get_corrected_index(int index) const;
    void compute_label_ranks(const TransitionSystem *transition_system,
                             std::vector<int> &label_ranks) const;
protected:
    virtual void dump_strategy_specific_options() const override {}
public:
    MergeDFP();
    virtual ~MergeDFP() override = default;
    virtual void initialize(const std::shared_ptr<AbstractTask> task) override;

    virtual std::pair<int, int> get_next(const std::vector<TransitionSystem *> &all_transition_systems) override;
    virtual std::string name() const override;
};

#endif

#ifndef MERGE_AND_SHRINK_MERGE_DFP_H
#define MERGE_AND_SHRINK_MERGE_DFP_H

#include "merge_strategy.h"

class Options;

class MergeDFP : public MergeStrategy {
    // border_atomic_composites is the first index at which a composite
    // transition system can be found in vector of all transition systems as passed
    // as argument to the get_next method.
    int border_atomics_composites;
    int get_corrected_index(int index) const;
protected:
    virtual void dump_strategy_specific_options() const {}
public:
    MergeDFP();
    virtual ~MergeDFP() {}

    // Note: all_transition_systems should be a vector of const TransitionSystem*, but
    // for the moment, compute_label_ranks is a non-const method because it
    // possibly needs to normalize and/or compute distances of some
    // transition systems. This should change when transition systems are always valid.
    virtual std::pair<int, int> get_next(const std::vector<TransitionSystem *> &all_transition_systems);
    virtual std::string name() const;
};

#endif

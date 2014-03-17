#ifndef MERGE_AND_SHRINK_SHRINK_MERGE_DFP_H
#define MERGE_AND_SHRINK_SHRINK_MERGE_DFP_H

#include "merge_strategy.h"

class Options;

class MergeDFP : public MergeStrategy {
    int remaining_merges;
    // border_atomic_composites is the first index at which a composite
    // abstraction can be found in vector of all abstractions as passed
    // as argument to the get_next method.
    std::size_t border_atomics_composites;

    std::size_t get_corrected_index(int index) const;
protected:
    virtual void dump_strategy_specific_options() const {}
public:
    MergeDFP();
    virtual ~MergeDFP() {}

    virtual bool done() const;
    virtual std::pair<int, int> get_next(const std::vector<Abstraction *> &all_abstractions);
    virtual std::string name() const;
};

#endif

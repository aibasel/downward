#ifndef MERGE_AND_SHRINK_MERGE_DFP_H
#define MERGE_AND_SHRINK_MERGE_DFP_H

#include "merge_strategy.h"

class Options;

class MergeDFP : public MergeStrategy {
    int remaining_merges;
    // indices_order emulates the previous behavior (when abstractions were
    // replaced in the vector of all abstractions rather than appended) by
    // storing indices in the corresponding order (a composite abstraction
    // is stored at the index of the first abstraction of its components).
    std::vector<std::size_t> indices_order;
protected:
    virtual void dump_strategy_specific_options() const {}
public:
    MergeDFP();
    virtual ~MergeDFP() {}

    virtual bool done() const;
    // Note: all_abstractions should be a vector of const Abstaction*, but
    // for the moment, compute_label_ranks is a non-const method because it
    // possibly needs to normalize and/or compute distances of some
    // abstractions. This should change when abstractions are always valid.
    virtual std::pair<int, int> get_next(const std::vector<Abstraction *> &all_abstractions);
    virtual std::string name() const;
};

#endif

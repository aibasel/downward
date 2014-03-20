#ifndef MERGE_AND_SHRINK_MERGE_DFP_H
#define MERGE_AND_SHRINK_MERGE_DFP_H

#include "merge_strategy.h"

class Options;

class MergeDFP : public MergeStrategy {
    enum AbstractionOrder {
        ALL_COMPOSITES_THEN_ATOMICS,
        EMULATE_PREVIOUS
    };
    AbstractionOrder abstraction_order;
    int remaining_merges;

    // MOST_RECENT_COMPOSITES_FIRST
    // border_atomic_composites is the first index at which a composite
    // abstraction can be found in vector of all abstractions as passed
    // as argument to the get_next method.
    std::size_t border_atomics_composites;
    std::size_t get_corrected_index(int index) const;

    // EMULATE_PREVIOUS_ABSTRACTION_ORDER
    // indices_order emulates the previous behavior (when abstractions were
    // replaced in the vector of all abstractions rather than appended) by
    // storing indices in the corresponding order (a composite abstraction
    // is stored at the index of the first abstraction of its components).
    std::vector<std::size_t> indices_order;
protected:
    virtual void dump_strategy_specific_options() const {}
public:
    explicit MergeDFP(const Options &opts);
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

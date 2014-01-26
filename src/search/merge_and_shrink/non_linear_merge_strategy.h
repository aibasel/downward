#ifndef MERGE_AND_SHRINK_SHRINK_NON_LINEAR_MERGE_STRATEGY_H
#define MERGE_AND_SHRINK_SHRINK_NON_LINEAR_MERGE_STRATEGY_H

#include "merge_strategy.h"

class Options;

class NonLinearMergeStrategy : public MergeStrategy {
    enum NonLinearMergeStrategyType {
        MERGE_NON_LINEAR_DFP
    };
    NonLinearMergeStrategyType non_linear_merge_strategy_type;
    int remaining_merges;
    int pair_weights_unequal_zero_counter;

    void dump_strategy_specific_options() const;
public:
    explicit NonLinearMergeStrategy(const Options &opts);

    virtual bool done() const;
    virtual void get_next(const std::vector<Abstraction *> &all_abstractions, std::pair<int, int> &next_indices);
    virtual std::string name() const;
};

#endif

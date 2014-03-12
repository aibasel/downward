#ifndef MERGE_AND_SHRINK_SHRINK_NON_LINEAR_MERGE_STRATEGY_H
#define MERGE_AND_SHRINK_SHRINK_NON_LINEAR_MERGE_STRATEGY_H

#include "merge_strategy.h"

class Options;

class NonLinearMergeStrategy : public MergeStrategy {
    enum NonLinearMergeStrategyType {
        DFP
    };
    NonLinearMergeStrategyType non_linear_merge_strategy_type;
    int remaining_merges;

    void dump_strategy_specific_options() const;
public:
    explicit NonLinearMergeStrategy(const Options &opts);
    virtual ~NonLinearMergeStrategy() {}

    virtual bool done() const;
    virtual std::pair<int, int> get_next(const std::vector<Abstraction *> &all_abstractions);
    virtual std::string name() const;
};

#endif

#ifndef MERGE_AND_SHRINK_SHRINK_LINEAR_MERGE_STRATEGY_H
#define MERGE_AND_SHRINK_SHRINK_LINEAR_MERGE_STRATEGY_H

#include "merge_strategy.h"

#include "variable_order_finder.h"

class Options;

class LinearMergeStrategy : public MergeStrategy {
    VariableOrderFinder order;
    int first_index;

    void dump_strategy_specific_options() const;
public:
    explicit LinearMergeStrategy(const Options &opts);
    virtual ~LinearMergeStrategy() {}

    virtual bool done() const;
    virtual std::pair<int, int> get_next(const std::vector<Abstraction *> &all_abstractions);
    virtual std::string name() const;
};

#endif

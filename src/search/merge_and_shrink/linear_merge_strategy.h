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

    virtual bool done() const;
    virtual void get_next(const std::vector<Abstraction *> &all_abstractions, std::pair<int, int> &next_indices);
    virtual std::string name() const;
    virtual void print_summary() const {}
};

#endif

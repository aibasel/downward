#ifndef MERGE_AND_SHRINK_SHRINK_LINEAR_MERGE_STRATEGY_H
#define MERGE_AND_SHRINK_SHRINK_LINEAR_MERGE_STRATEGY_H

#include "variable_order_finder.h"

class LinearMergeStrategy : public MergeStrategy {
    VariableOrderFinder order;
    int previous_index;
public:
    LinearMergeStrategy(const MergeStrategyEnum &merge_strategy_enum);

    virtual bool done() const;
    virtual std::pair<int, int> get_next(const std::vector<Abstraction *> &all_abstractions);
};

#endif

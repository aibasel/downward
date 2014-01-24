#ifndef NON_LINEAR_MERGE_STRATEGY_H
#define NON_LINEAR_MERGE_STRATEGY_H

#include "merge_strategy.h"

class NonLinearMergeStrategy : public MergeStrategy {
public:
    NonLinearMergeStrategy(const MergeStrategyEnum &merge_strategy_enum);

    virtual bool done() const;
    virtual std::pair<int, int> get_next(const std::vector<Abstraction *> &all_abstractions);
};

#endif // NON_LINEAR_MERGE_STRATEGY_H

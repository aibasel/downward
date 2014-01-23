#ifndef MERGE_AND_SHRINK_SHRINK_MERGE_STRATEGY_H
#define MERGE_AND_SHRINK_SHRINK_MERGE_STRATEGY_H

#include <vector>

class Abstraction;

enum MergeStrategyEnum {
    MERGE_LINEAR_CG_GOAL_LEVEL,
    MERGE_LINEAR_CG_GOAL_RANDOM,
    MERGE_LINEAR_GOAL_CG_LEVEL,
    MERGE_LINEAR_RANDOM,
    MERGE_DFP,
    MERGE_LINEAR_LEVEL,
    MERGE_LINEAR_REVERSE_LEVEL
};

class MergeStrategy {
protected:
    const MergeStrategyEnum merge_strategy_enum;
public:
    MergeStrategy(const MergeStrategyEnum &merge_strategy_enum);

    virtual bool done() const = 0;
    virtual std::pair<int, int> get_next(const std::vector<Abstraction *> &all_abstractions) = 0;
};

#endif

#ifndef NON_LINEAR_MERGE_STRATEGY_H
#define NON_LINEAR_MERGE_STRATEGY_H

#include "merge_strategy.h"

class Options;

class NonLinearMergeStrategy : public MergeStrategy {
    void dump_strategy_specific_options() const;
public:
    explicit NonLinearMergeStrategy(const Options &opts);

    virtual bool done() const;
    virtual std::pair<int, int> get_next(const std::vector<Abstraction *> &all_abstractions);
    virtual std::string name() const;
};

#endif // NON_LINEAR_MERGE_STRATEGY_H

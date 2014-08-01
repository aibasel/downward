#ifndef MERGE_AND_SHRINK_SHRINK_MERGE_LINEAR_H
#define MERGE_AND_SHRINK_SHRINK_MERGE_LINEAR_H

#include "merge_strategy.h"

#include "variable_order_finder.h"

class Options;

class MergeLinear : public MergeStrategy {
    VariableOrderFinder order;
    bool need_first_index;
protected:
    virtual void dump_strategy_specific_options() const;
public:
    explicit MergeLinear(const Options &opts);
    virtual ~MergeLinear() {}

    virtual std::pair<int, int> get_next(const std::vector<Abstraction *> &all_abstractions);
    virtual std::string name() const;
};

#endif

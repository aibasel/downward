#ifndef MERGE_AND_SHRINK_MERGE_LINEAR_H
#define MERGE_AND_SHRINK_MERGE_LINEAR_H

#include "merge_strategy.h"

#include <memory>

class VariableOrderFinder;

namespace merge_and_shrink {
class MergeLinear : public MergeStrategy {
    std::unique_ptr<VariableOrderFinder> variable_order_finder;
    bool need_first_index;
public:
    MergeLinear(FactoredTransitionSystem &fts,
                std::unique_ptr<VariableOrderFinder> variable_order_finder);
    virtual ~MergeLinear() override = default;
    virtual std::pair<int, int> get_next() override;
};
}

#endif

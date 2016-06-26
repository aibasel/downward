#ifndef MERGE_AND_SHRINK_MERGE_STATELESS_H
#define MERGE_AND_SHRINK_MERGE_STATELESS_H

#include "merge_strategy.h"

#include <memory>

namespace merge_and_shrink {
class MergeSelector;
class MergeStrategyStateless : public MergeStrategy {
    std::shared_ptr<MergeSelector> merge_selector;
public:
    MergeStrategyStateless(
        FactoredTransitionSystem &fts,
        std::shared_ptr<MergeSelector> merge_selector);
    virtual ~MergeStrategyStateless() override = default;
    virtual std::pair<int, int> get_next() override;
};
}

#endif

#ifndef MERGE_AND_SHRINK_MERGE_STRATEGY_PRECOMPUTED_H
#define MERGE_AND_SHRINK_MERGE_STRATEGY_PRECOMPUTED_H

#include "merge_strategy.h"

#include <memory>

namespace merge_and_shrink {
class MergeTree;
class MergeStrategyPrecomputed : public MergeStrategy {
    std::unique_ptr<MergeTree> merge_tree;
public:
    MergeStrategyPrecomputed(
        const FactoredTransitionSystem &fts,
        std::unique_ptr<MergeTree> merge_tree);
    virtual ~MergeStrategyPrecomputed() override = default;
    virtual std::pair<int, int> get_next() override;
};
}

#endif

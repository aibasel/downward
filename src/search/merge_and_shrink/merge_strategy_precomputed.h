#ifndef MERGE_AND_SHRINK_MERGE_PRECOMPUTED_H
#define MERGE_AND_SHRINK_MERGE_PRECOMPUTED_H

#include "merge_strategy.h"

namespace merge_and_shrink {
class MergeTree;
class MergeStrategyPrecomputed : public MergeStrategy {
    MergeTree *merge_tree;
public:
    MergeStrategyPrecomputed(
        FactoredTransitionSystem &fts, MergeTree *merge_tree);
    virtual ~MergeStrategyPrecomputed();
    virtual std::pair<int, int> get_next() override;
};
}

#endif

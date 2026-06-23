#ifndef MERGE_AND_SHRINK_MERGE_STRATEGY_SCCS_H
#define MERGE_AND_SHRINK_MERGE_STRATEGY_SCCS_H

#include "merge_strategy.h"

#include <memory>
#include <vector>

namespace merge_and_shrink {
class MergeSelector;
class MergeStrategySCCs : public MergeStrategy {
    std::shared_ptr<MergeSelector> merge_selector;
    std::vector<std::vector<int>> unfinished_clusters;
    bool allow_working_on_all_clusters;
public:
    MergeStrategySCCs(
        const FactoredTransitionSystem &fts,
        const std::shared_ptr<MergeSelector> &merge_selector,
        std::vector<std::vector<int>> &&unfinished_clusters,
        bool allow_working_on_all_clusters);
    virtual ~MergeStrategySCCs() override;
    virtual std::pair<int, int> get_next() override;
};
}

#endif

#ifndef MERGE_AND_SHRINK_MERGE_STRATEGY_SCCS_H
#define MERGE_AND_SHRINK_MERGE_STRATEGY_SCCS_H

#include "merge_strategy.h"

#include <memory>
#include <vector>

namespace merge_and_shrink {
class MergeSelector;
class MergeStrategySCCs : public MergeStrategy {
    std::shared_ptr<MergeSelector> merge_selector;
    std::vector<std::vector<int>> non_singleton_cg_sccs;

    std::vector<int> current_ts_indices;
public:
    MergeStrategySCCs(
        const FactoredTransitionSystem &fts,
        const std::shared_ptr<MergeSelector> &merge_selector,
        std::vector<std::vector<int>> &&non_singleton_cg_sccs);
    virtual ~MergeStrategySCCs() override;
    virtual std::pair<int, int> get_next() override;
};
}

#endif

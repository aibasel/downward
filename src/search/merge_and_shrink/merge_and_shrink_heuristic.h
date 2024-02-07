#ifndef MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H
#define MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H

#include "merge_strategy_factory.h"
#include "shrink_strategy.h"
#include "label_reduction.h"

#include "../heuristic.h"

#include <memory>

namespace merge_and_shrink {
class FactoredTransitionSystem;
class MergeAndShrinkRepresentation;

class MergeAndShrinkHeuristic : public Heuristic {
    // The final merge-and-shrink representations, storing goal distances.
    std::vector<std::unique_ptr<MergeAndShrinkRepresentation>> mas_representations;

    void extract_factor(FactoredTransitionSystem &fts, int index);
    bool extract_unsolvable_factor(FactoredTransitionSystem &fts);
    void extract_nontrivial_factors(FactoredTransitionSystem &fts);
    void extract_factors(FactoredTransitionSystem &fts);
protected:
    virtual int compute_heuristic(const State &ancestor_state) override;
public:
    MergeAndShrinkHeuristic(
            const std::shared_ptr<MergeStrategyFactory> &merge_strategy,
            const std::shared_ptr<ShrinkStrategy> &shrink_strategy,
            const std::shared_ptr<LabelReduction> &label_reduction,
            int max_states,
            int max_states_before_merge,
            int threshold_before_merge,
            bool prune_unreachable_states,
            bool prune_irrelevant_states,
            double main_loop_max_time,
            const std::shared_ptr<AbstractTask> &transform,
            bool cache_estimates,
            const std::string &description,
            utils::Verbosity verbosity
            );
};
}

#endif

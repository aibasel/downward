#ifndef MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_MIASM_H
#define MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_MIASM_H

#include "merge_scoring_function.h"

#include "../utils/logging.h"

#include <memory>
#include <optional>

namespace merge_and_shrink {
class ShrinkStrategy;
class MergeScoringFunctionMIASM : public MergeScoringFunction {
    const bool use_caching;
    std::shared_ptr<ShrinkStrategy> shrink_strategy;
    const int max_states;
    const int max_states_before_merge;
    const int shrink_threshold_before_merge;
    utils::LogProxy silent_log;
    std::vector<std::vector<std::optional<double>>> cached_scores_by_merge_candidate_indices;

    virtual std::string name() const override;
    virtual void dump_function_specific_options(utils::LogProxy &log) const override;
public:
    MergeScoringFunctionMIASM(
        std::shared_ptr<ShrinkStrategy> shrink_strategy,
        int max_states, int max_states_before_merge,
        int threshold_before_merge, bool use_caching);
    virtual std::vector<double> compute_scores(
        const FactoredTransitionSystem &fts,
        const std::vector<std::pair<int, int>> &merge_candidates) override;
    virtual void initialize(const TaskProxy &task_proxy) override;

    virtual bool requires_init_distances() const override {
        return true;
    }

    virtual bool requires_goal_distances() const override {
        return true;
    }
};
}

#endif

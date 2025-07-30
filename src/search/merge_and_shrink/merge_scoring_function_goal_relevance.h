#ifndef MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_GOAL_RELEVANCE_H
#define MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_GOAL_RELEVANCE_H

#include "merge_scoring_function.h"

namespace merge_and_shrink {
class MergeScoringFunctionGoalRelevance : public MergeScoringFunction {
    virtual std::string name() const override;
public:
    MergeScoringFunctionGoalRelevance() = default;
    virtual std::vector<double> compute_scores(
        const FactoredTransitionSystem &fts,
        const std::vector<std::pair<int, int>> &merge_candidates) override;

    virtual bool requires_init_distances() const override {
        return false;
    }

    virtual bool requires_goal_distances() const override {
        return false;
    }
};
}

#endif

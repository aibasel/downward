#ifndef MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_GOAL_RELEVANCE_H
#define MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_GOAL_RELEVANCE_H

#include "merge_scoring_function.h"

namespace merge_and_shrink {
class MergeScoringFunctionGoalRelevance : public MergeScoringFunction {
protected:
    virtual std::string name() const override;
public:
    MergeScoringFunctionGoalRelevance() = default;
    virtual ~MergeScoringFunctionGoalRelevance() override = default;
    virtual std::vector<double> compute_scores(
        FactoredTransitionSystem &fts,
        const std::vector<std::pair<int, int>> &merge_candidates) override;
};
}

#endif

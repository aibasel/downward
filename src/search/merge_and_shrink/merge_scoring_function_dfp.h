#ifndef MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_DFP_H
#define MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_DFP_H

#include "merge_scoring_function.h"

namespace merge_and_shrink {
class TransitionSystem;
class MergeScoringFunctionDFP : public MergeScoringFunction {
    std::vector<int> compute_label_ranks(
        const FactoredTransitionSystem &fts, int index) const;
protected:
    virtual std::string name() const override;
public:
    MergeScoringFunctionDFP() = default;
    virtual ~MergeScoringFunctionDFP() override = default;
    virtual std::vector<int> compute_scores(
        FactoredTransitionSystem &fts,
        const std::vector<std::pair<int, int>> &merge_candidates) override;
};
}

#endif

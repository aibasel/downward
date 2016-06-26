#ifndef MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_TIEBREAKING_H
#define MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_TIEBREAKING_H

#include "merge_scoring_function.h"

namespace options {
class Options;
}

namespace utils {
class RandomNumberGenerator;
}

namespace merge_and_shrink {
class TransitionSystem;
class MergeScoringFunctionTiebreakingRandom : public MergeScoringFunction {
    int random_seed; // only for dump options
    std::shared_ptr<utils::RandomNumberGenerator> rng;
protected:
    virtual std::string name() const override {
        return "random tiebreaking";
    }
    virtual void dump_specific_options() const override;
public:
    explicit MergeScoringFunctionTiebreakingRandom(const options::Options &options);
    virtual std::vector<int> compute_scores(
        FactoredTransitionSystem &fts,
        const std::vector<std::pair<int, int>> &merge_candidates) override;
};
}

#endif

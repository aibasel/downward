#ifndef MERGE_AND_SHRINK_FACTOR_SCORING_FUNCTIONS_H
#define MERGE_AND_SHRINK_FACTOR_SCORING_FUNCTIONS_H

#include <memory>
#include <vector>

namespace options {
class Options;
}

namespace utils {
class RandomNumberGenerator;
}

namespace merge_and_shrink {
class FactoredTransitionSystem;

class FactorScoringFunction {
public:
    virtual ~FactorScoringFunction() = default;
    virtual std::vector<int> compute_scores(
        const FactoredTransitionSystem &fts,
        const std::vector<int> &indices) const = 0;
};

class FactorScoringFunctionInitH : public FactorScoringFunction {
public:
    FactorScoringFunctionInitH() = default;
    virtual ~FactorScoringFunctionInitH() override = default;
    virtual std::vector<int> compute_scores(
        const FactoredTransitionSystem &fts,
        const std::vector<int> &indices) const override;
};

class FactorScoringFunctionSize : public FactorScoringFunction {
public:
    FactorScoringFunctionSize() = default;
    virtual ~FactorScoringFunctionSize() override = default;
    virtual std::vector<int> compute_scores(
        const FactoredTransitionSystem &fts,
        const std::vector<int> &indices) const override;
};

class FactorScoringFunctionRandom : public FactorScoringFunction {
    std::shared_ptr<utils::RandomNumberGenerator> rng;
public:
    explicit FactorScoringFunctionRandom(const options::Options &options);
    virtual ~FactorScoringFunctionRandom() override = default;
    virtual std::vector<int> compute_scores(
        const FactoredTransitionSystem &fts,
        const std::vector<int> &indices) const override;
};
}

#endif

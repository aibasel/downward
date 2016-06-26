#ifndef MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_H
#define MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_H

#include <memory>
#include <vector>

class AbstractTask;

namespace merge_and_shrink {
class FactoredTransitionSystem;
class MergeScoringFunction {
protected:
    virtual std::string name() const = 0;
    virtual void dump_specific_options() const {}
public:
    MergeScoringFunction() = default;
    virtual std::vector<int> compute_scores(
        FactoredTransitionSystem &fts,
        const std::vector<std::pair<int, int>> &merge_candidates) = 0;
    virtual void initialize(std::shared_ptr<AbstractTask>) {}
    void dump_options() const;
};
}

#endif

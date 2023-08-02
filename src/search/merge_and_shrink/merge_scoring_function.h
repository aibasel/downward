#ifndef MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_H
#define MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_H

#include <string>
#include <vector>

class TaskProxy;

namespace utils {
class LogProxy;
}

namespace merge_and_shrink {
class FactoredTransitionSystem;
class MergeScoringFunction {
    virtual std::string name() const = 0;
    virtual void dump_function_specific_options(utils::LogProxy &) const {}
protected:
    bool initialized;
public:
    MergeScoringFunction();
    virtual ~MergeScoringFunction() = default;
    virtual std::vector<double> compute_scores(
        const FactoredTransitionSystem &fts,
        const std::vector<std::pair<int, int>> &merge_candidates) = 0;
    virtual bool requires_init_distances() const = 0;
    virtual bool requires_goal_distances() const = 0;

    // Overriding methods must set initialized to true.
    virtual void initialize(const TaskProxy &) {
        initialized = true;
    }

    void dump_options(utils::LogProxy &log) const;
};
}

#endif

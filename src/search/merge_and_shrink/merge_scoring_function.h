#ifndef MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_H
#define MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_H

#include "../utils/logging.h"

#include <string>
#include <vector>

class TaskProxy;

namespace options {
class OptionParser;
class Options;
}

namespace merge_and_shrink {
class FactoredTransitionSystem;

class MergeScoringFunction {
protected:
    mutable utils::LogProxy log;
    bool initialized;

    virtual std::string name() const = 0;
    virtual void dump_function_specific_options() const {}
public:
    explicit MergeScoringFunction(const options::Options &options);
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

    void dump_options() const;
};

extern void add_merge_scoring_function_options_to_parser(options::OptionParser &parser);
}

#endif

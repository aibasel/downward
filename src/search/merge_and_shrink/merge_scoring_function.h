#ifndef MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_H
#define MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class TaskProxy;

namespace plugins {
class Feature;
class Options;
}

namespace utils {
class LogProxy;
}

namespace merge_and_shrink {
class FactoredTransitionSystem;
struct MergeCandidate {
    int id;
    int index1;
    int index2;
    MergeCandidate(int id, int index1, int index2)
        : id(id), index1(index1), index2(index2) {
    }
};

class MergeScoringFunction {
protected:
    const bool use_caching;
    bool initialized;
    std::unordered_map<int, double> cached_scores;

    virtual std::string name() const = 0;
    virtual void dump_function_specific_options(utils::LogProxy &) const {}
public:
    explicit MergeScoringFunction(const plugins::Options &options);
    virtual ~MergeScoringFunction() = default;
    virtual std::vector<double> compute_scores(
        const FactoredTransitionSystem &fts,
        const std::vector<MergeCandidate> &merge_candidates) = 0;
    virtual bool requires_init_distances() const = 0;
    virtual bool requires_goal_distances() const = 0;

    // Overriding methods must set initialized to true.
    virtual void initialize(const TaskProxy &) {
        initialized = true;
    }

    void dump_options(utils::LogProxy &log) const;
};

extern void add_merge_scoring_function_options_to_feature(plugins::Feature &feature);
}

#endif

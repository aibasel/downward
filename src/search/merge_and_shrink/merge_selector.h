#ifndef MERGE_AND_SHRINK_MERGE_SELECTOR_H
#define MERGE_AND_SHRINK_MERGE_SELECTOR_H

#include <optional>
#include <string>
#include <vector>

class TaskProxy;

namespace utils {
class LogProxy;
}

namespace merge_and_shrink {
class FactoredTransitionSystem;
struct MergeCandidate;
class MergeSelector {
protected:
    std::vector<std::vector<std::optional<MergeCandidate>>> merge_candidates_by_indices;
    int num_candidates;

    MergeCandidate get_candidate(int index1, int index2);
    virtual std::string name() const = 0;
    virtual void dump_selector_specific_options(utils::LogProxy &) const {}
    std::vector<MergeCandidate> compute_merge_candidates(
        const FactoredTransitionSystem &fts,
        const std::vector<int> &indices_subset);
public:
    MergeSelector() = default;
    virtual ~MergeSelector() = default;
    virtual std::pair<int, int> select_merge(
        const FactoredTransitionSystem &fts,
        const std::vector<int> &indices_subset = std::vector<int>()) = 0;
    virtual void initialize(const TaskProxy &task_proxy);
    void dump_options(utils::LogProxy &log) const;
    virtual bool requires_init_distances() const = 0;
    virtual bool requires_goal_distances() const = 0;
};
}

#endif

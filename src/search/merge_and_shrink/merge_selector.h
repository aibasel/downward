#ifndef MERGE_AND_SHRINK_MERGE_SELECTOR_H
#define MERGE_AND_SHRINK_MERGE_SELECTOR_H

#include <string>
#include <vector>

class TaskProxy;

namespace utils {
class LogProxy;
}

namespace merge_and_shrink {
class FactoredTransitionSystem;
class MergeSelector {
private:
    std::vector<std::pair<int, int>> compute_merge_candidates(
        const FactoredTransitionSystem &fts) const;

protected:
    virtual std::string name() const = 0;
    virtual void dump_selector_specific_options(utils::LogProxy &) const {
    }
    
public:
    MergeSelector() = default;
    virtual ~MergeSelector() = default;
    // Select a merge candidate from all possible candidates.
    std::pair<int, int> select_merge(const FactoredTransitionSystem &fts) const;
    /*
     * Select a merge candidate from the given candidates, which must be valid 
     * candidates for the given FTS.
     */
    virtual std::pair<int, int> select_merge_from_candidates(
        const FactoredTransitionSystem &fts,
        std::vector<std::pair<int, int>> &&merge_candidates) const = 0;
    virtual void initialize(const TaskProxy &task_proxy) = 0;
    void dump_options(utils::LogProxy &log) const;
    virtual bool requires_init_distances() const = 0;
    virtual bool requires_goal_distances() const = 0;
};
}

#endif

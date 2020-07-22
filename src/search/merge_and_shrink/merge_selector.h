#ifndef MERGE_AND_SHRINK_MERGE_SELECTOR_H
#define MERGE_AND_SHRINK_MERGE_SELECTOR_H

#include "../utils/logging.h"

#include <string>
#include <vector>

class TaskProxy;

namespace options {
class Options;
}

namespace merge_and_shrink {
class FactoredTransitionSystem;

class MergeSelector {
protected:
    mutable utils::LogProxy log;

    virtual std::string name() const = 0;
    virtual void dump_specific_options() const = 0;
    std::vector<std::pair<int, int>> compute_merge_candidates(
        const FactoredTransitionSystem &fts,
        const std::vector<int> &indices_subset) const;
public:
    explicit MergeSelector(const options::Options &options);
    virtual ~MergeSelector() = default;
    virtual std::pair<int, int> select_merge(
        const FactoredTransitionSystem &fts,
        const std::vector<int> &indices_subset = std::vector<int>()) const = 0;
    virtual void initialize(const TaskProxy &task_proxy) = 0;
    void dump_options() const;
    virtual bool requires_init_distances() const = 0;
    virtual bool requires_goal_distances() const = 0;
};

extern void add_merge_selector_options_to_parser(options::OptionParser &parser);
}

#endif

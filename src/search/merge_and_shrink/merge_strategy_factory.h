#ifndef MERGE_AND_SHRINK_MERGE_STRATEGY_FACTORY_H
#define MERGE_AND_SHRINK_MERGE_STRATEGY_FACTORY_H

#include <memory>
#include <string>

class TaskProxy;

namespace utils {
class LogProxy;
}

namespace merge_and_shrink {
class FactoredTransitionSystem;
class MergeStrategy;

class MergeStrategyFactory {
protected:
    virtual std::string name() const = 0;
    virtual void dump_strategy_specific_options(utils::LogProxy &log) const = 0;
public:
    MergeStrategyFactory() = default;
    virtual ~MergeStrategyFactory() = default;
    void dump_options(utils::LogProxy &log) const;
    virtual std::unique_ptr<MergeStrategy> compute_merge_strategy(
        const TaskProxy &task_proxy,
        const FactoredTransitionSystem &fts,
        utils::LogProxy &log) = 0;
    virtual bool requires_init_distances() const = 0;
    virtual bool requires_goal_distances() const = 0;
};
}

#endif

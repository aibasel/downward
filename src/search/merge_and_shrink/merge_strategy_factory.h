#ifndef MERGE_AND_SHRINK_MERGE_STRATEGY_FACTORY_H
#define MERGE_AND_SHRINK_MERGE_STRATEGY_FACTORY_H

#include "../component.h"

#include "../utils/logging.h"

#include <memory>
#include <string>

class TaskProxy;

namespace plugins {
class Options;
class Feature;
}

namespace merge_and_shrink {
class FactoredTransitionSystem;
class MergeStrategy;

class MergeStrategyFactory : public components::TaskSpecificComponent {
protected:
    mutable utils::LogProxy log;

    virtual std::string name() const = 0;
    virtual void dump_strategy_specific_options() const = 0;
public:
    MergeStrategyFactory(
        const std::shared_ptr<AbstractTask> &task, utils::Verbosity verbosity);
    void dump_options() const;
    virtual std::unique_ptr<MergeStrategy> compute_merge_strategy(
        const TaskProxy &task_proxy, const FactoredTransitionSystem &fts) = 0;
    virtual bool requires_init_distances() const = 0;
    virtual bool requires_goal_distances() const = 0;
};

using TaskIndependentMergeStrategyFactory = components::TaskIndependentComponent<MergeStrategyFactory>;

extern void add_merge_strategy_options_to_feature(plugins::Feature &feature);
extern std::tuple<utils::Verbosity> get_merge_strategy_arguments_from_options(
    const plugins::Options &opts);
}

#endif

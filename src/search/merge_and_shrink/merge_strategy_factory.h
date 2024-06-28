#ifndef MERGE_AND_SHRINK_MERGE_STRATEGY_FACTORY_H
#define MERGE_AND_SHRINK_MERGE_STRATEGY_FACTORY_H

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

class MergeStrategyFactory {
protected:
    mutable utils::LogProxy log;

    virtual std::string name() const = 0;
    virtual void dump_strategy_specific_options() const = 0;
public:
    MergeStrategyFactory(utils::Verbosity verbosity);
    virtual ~MergeStrategyFactory() = default;
    void dump_options() const;
    virtual std::unique_ptr<MergeStrategy> compute_merge_strategy(
        const TaskProxy &task_proxy,
        const FactoredTransitionSystem &fts) = 0;
    virtual bool requires_init_distances() const = 0;
    virtual bool requires_goal_distances() const = 0;
};

extern void add_merge_strategy_options_to_feature(plugins::Feature &feature);
extern std::tuple<utils::Verbosity>
get_merge_strategy_arguments_from_options(
    const plugins::Options &opts);
}

#endif

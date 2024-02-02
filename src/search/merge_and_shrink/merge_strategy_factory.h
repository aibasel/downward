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
    const std::string name;
protected:
    mutable utils::LogProxy log;

    virtual std::string type() const = 0;
    virtual void dump_strategy_specific_options() const = 0;
public:
    MergeStrategyFactory(
            const std::string &name,
            utils::Verbosity verbosity);
    explicit MergeStrategyFactory(const plugins::Options &options); // TODO issue1082 remove
    virtual ~MergeStrategyFactory() = default;
    void dump_options() const;
    virtual std::unique_ptr<MergeStrategy> compute_merge_strategy(
        const TaskProxy &task_proxy,
        const FactoredTransitionSystem &fts) = 0;
    virtual bool requires_init_distances() const = 0;
    virtual bool requires_goal_distances() const = 0;
};

    extern void add_merge_strategy_options_to_feature(plugins::Feature &feature, const std::string &name);
    extern void add_merge_strategy_options_to_feature(plugins::Feature &feature); // TODO issu1082 remove
}

#endif

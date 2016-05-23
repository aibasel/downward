#ifndef MERGE_AND_SHRINK_MERGE_STRATEGY_FACTORY_H
#define MERGE_AND_SHRINK_MERGE_STRATEGY_FACTORY_H

#include <memory>
#include <string>

class AbstractTask;

namespace merge_and_shrink {
class FactoredTransitionSystem;
class MergeStrategy;

class MergeStrategyFactory {
protected:
    virtual void dump_strategy_specific_options() const = 0;
public:
    MergeStrategyFactory() = default;
    virtual ~MergeStrategyFactory() = default;
    void dump_options() const;
    virtual std::unique_ptr<MergeStrategy> compute_merge_strategy(
        const std::shared_ptr<AbstractTask> &task,
        FactoredTransitionSystem &fts) = 0;
    virtual std::string name() const = 0;
};
}

#endif

#ifndef MERGE_AND_SHRINK_MERGE_STRATEGY_FACTORY_STATELESS_H
#define MERGE_AND_SHRINK_MERGE_STRATEGY_FACTORY_STATELESS_H

#include "merge_strategy_factory.h"

namespace merge_and_shrink {
class MergeSelector;

class MergeStrategyFactoryStateless : public MergeStrategyFactory {
    std::shared_ptr<MergeSelector> merge_selector;
protected:
    virtual std::string name() const override;
    virtual void dump_strategy_specific_options() const override;
public:
    MergeStrategyFactoryStateless(
        const std::shared_ptr<MergeSelector> &merge_selector,
        utils::Verbosity verbosity);
    virtual std::unique_ptr<MergeStrategy> compute_merge_strategy(
        const TaskProxy &task_proxy,
        const FactoredTransitionSystem &fts) override;
    virtual bool requires_init_distances() const override;
    virtual bool requires_goal_distances() const override;
};
}

#endif

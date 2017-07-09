#ifndef MERGE_AND_SHRINK_MERGE_STRATEGY_FACTORY_PRECOMPUTED_H
#define MERGE_AND_SHRINK_MERGE_STRATEGY_FACTORY_PRECOMPUTED_H

#include "merge_strategy_factory.h"

namespace options {
class Options;
}

namespace merge_and_shrink {
class MergeTreeFactory;
class MergeStrategyFactoryPrecomputed : public MergeStrategyFactory {
    std::shared_ptr<MergeTreeFactory> merge_tree_factory;
protected:
    virtual std::string name() const override;
    virtual void dump_strategy_specific_options() const override;
public:
    explicit MergeStrategyFactoryPrecomputed(options::Options &options);
    virtual ~MergeStrategyFactoryPrecomputed() override = default;
    virtual std::unique_ptr<MergeStrategy> compute_merge_strategy(
        const TaskProxy &task_proxy,
        const FactoredTransitionSystem &fts) override;
    virtual bool requires_init_distances() const override;
    virtual bool requires_goal_distances() const override;
};
}

#endif

#ifndef MERGE_AND_SHRINK_MERGE_STRATEGY_FACTORY_SCCS_H
#define MERGE_AND_SHRINK_MERGE_STRATEGY_FACTORY_SCCS_H

#include "merge_strategy_factory.h"

namespace options {
class Options;
}

namespace merge_and_shrink {
class MergeTreeFactory;
class MergeSelector;
class MergeStrategyFactorySCCs : public MergeStrategyFactory {
    enum class OrderOfSCCs {
        TOPOLOGICAL,
        REVERSE_TOPOLOGICAL,
        DECREASING,
        INCREASING
    };
    OrderOfSCCs order_of_sccs;
    std::shared_ptr<MergeTreeFactory> merge_tree_factory;
    std::shared_ptr<MergeSelector> merge_selector;
protected:
    virtual std::string name() const override;
    virtual void dump_strategy_specific_options() const override;
public:
    MergeStrategyFactorySCCs(const options::Options &options);
    virtual ~MergeStrategyFactorySCCs() override = default;
    virtual std::unique_ptr<MergeStrategy> compute_merge_strategy(
        const TaskProxy &task_proxy,
        const FactoredTransitionSystem &fts) override;

    virtual bool requires_init_distances() const override {
        return false;
    }

    virtual bool requires_goal_distances() const override {
        return false;
    }
};
}

#endif

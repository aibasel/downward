#ifndef MERGE_AND_SHRINK_MERGE_STRATEGY_FACTORY_LINEAR_H
#define MERGE_AND_SHRINK_MERGE_STRATEGY_FACTORY_LINEAR_H

#include "merge_strategy_factory.h"

#include "../variable_order_finder.h"

namespace merge_and_shrink {
class MergeStrategyFactoryLinear : public MergeStrategyFactory {
    VariableOrderType variable_order_type;
protected:
    virtual void dump_strategy_specific_options() const override;
public:
    explicit MergeStrategyFactoryLinear(options::Options &options);
    virtual ~MergeStrategyFactoryLinear() override = default;
    virtual std::unique_ptr<MergeStrategy> compute_merge_strategy(
        std::shared_ptr<AbstractTask> task,
        FactoredTransitionSystem &fts) override;
    virtual std::string name() const override;
};
}

#endif

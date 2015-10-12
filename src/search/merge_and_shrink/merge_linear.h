#ifndef MERGE_AND_SHRINK_MERGE_LINEAR_H
#define MERGE_AND_SHRINK_MERGE_LINEAR_H

#include "merge_strategy.h"

#include "../variable_order_finder.h"

#include <memory>

class Options;

class MergeLinear : public MergeStrategy {
    // Only needed until variable order finder is initialized.
    VariableOrderType variable_order_type;
    std::unique_ptr<VariableOrderFinder> variable_order_finder;
    bool need_first_index;
protected:
    virtual void dump_strategy_specific_options() const override;
public:
    explicit MergeLinear(const Options &opts);
    virtual ~MergeLinear() override = default;
    virtual void initialize(const std::shared_ptr<AbstractTask> task) override;

    virtual std::pair<int, int> get_next(const std::vector<TransitionSystem *> &all_transition_systems) override;
    virtual std::string name() const override;
};

#endif

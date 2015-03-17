#ifndef COST_ADAPTED_TASK_H
#define COST_ADAPTED_TASK_H

#include "delegating_task.h"
#include "operator_cost.h"

class Options;

class CostAdaptedTask : public DelegatingTask {
    const OperatorCost cost_type;
public:
    explicit CostAdaptedTask(const Options &opts);
    virtual ~CostAdaptedTask() override = default;

    virtual int get_operator_cost(int index, bool is_axiom) const override;
};

#endif

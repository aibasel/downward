#ifndef TASKS_COST_ADAPTED_TASK_H
#define TASKS_COST_ADAPTED_TASK_H

#include "delegating_task.h"

#include "../component_map.h"
#include "../operator_cost.h"

namespace tasks {
/*
  Task transformation that changes operator costs. If the parent task assigns
  costs 'c' to an operator, its adjusted costs, depending on the value of the
  cost_type option, are:

    NORMAL:  c
    ONE:     1
    PLUSONE: 1, if all operators have cost 1 in the parent task, else c + 1

  Regardless of the cost_type value, axioms will always keep their original
  cost, which is 0 by default.
*/
class CostAdaptedTask : public DelegatingTask {
    const OperatorCost cost_type;
    const bool parent_is_unit_cost;
public:
    CostAdaptedTask(
        const std::shared_ptr<AbstractTask> &parent,
        OperatorCost cost_type);
    virtual ~CostAdaptedTask() override = default;

    virtual int get_operator_cost(int index, bool is_axiom) const override;
};


class TaskIndependentCostAdaptedTask : public TaskIndependentDelegatingTask {
    const OperatorCost cost_type;
public:
    explicit TaskIndependentCostAdaptedTask(OperatorCost cost_type);
    virtual ~TaskIndependentCostAdaptedTask() override = default;

    virtual std::shared_ptr<DelegatingTask> create_task_specific_DelegatingTask(
        const std::shared_ptr<AbstractTask> &task,
        std::unique_ptr<ComponentMap> &component_map, int depth = -1) override;

    virtual std::shared_ptr<CostAdaptedTask> create_task_specific_CostAdaptedTask(const std::shared_ptr<AbstractTask> &task, int depth = -1);
    virtual std::shared_ptr<CostAdaptedTask> create_task_specific_CostAdaptedTask(const std::shared_ptr<AbstractTask> &task, std::unique_ptr<ComponentMap> &component_map, int depth = -1);
};
}

#endif

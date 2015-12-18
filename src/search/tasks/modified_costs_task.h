#ifndef TASKS_MODIFIED_COSTS_TASK_H
#define TASKS_MODIFIED_COSTS_TASK_H

#include "../delegating_task.h"

#include <vector>

namespace ExtraTasks {
class ModifiedCostsTask : public DelegatingTask {
    const std::vector<int> operator_costs;

public:
    ModifiedCostsTask(
        const std::shared_ptr<AbstractTask> parent,
        std::vector<int> &&costs);
    virtual ~ModifiedCostsTask() override = default;

    virtual int get_operator_cost(int index, bool is_axiom) const override;
};
}

#endif

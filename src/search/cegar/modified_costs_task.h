#ifndef MODIFIED_COSTS_TASK_H
#define MODIFIED_COSTS_TASK_H

#include "../delegating_task.h"

#include <vector>

class Options;

class ModifiedCostsTask : public DelegatingTask {
    const std::vector<int> operator_costs;
public:
    explicit ModifiedCostsTask(const Options &opts);
    virtual ~ModifiedCostsTask() override = default;

    virtual int get_operator_cost(int index, bool is_axiom) const override;
};

#endif

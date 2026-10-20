#ifndef PLAN_IMPROVEMENT_PLAN_IMPROVER_H
#define PLAN_IMPROVEMENT_PLAN_IMPROVER_H

#include "../abstract_task.h"
#include "../plan_manager.h"

#include <memory>

class TaskProxy;
class StateRegistry;

class PlanImprovement {
public:
    virtual Plan improve(
        const Plan &plan, const TaskProxy &task_proxy,
        StateRegistry &state_registry) = 0;

    virtual ~PlanImprovement() = default;
};

#endif
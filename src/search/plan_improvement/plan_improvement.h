#ifndef PLAN_IMPROVEMENT_H
#define PLAN_IMPROVEMENT_H

#include "../plan_manager.h"

#include <memory>

class TaskProxy;
class StateRegistry;

class PlanImprovement {
public:
    virtual Plan improve(
        const Plan &plan, const TaskProxy &task_proxy,
        StateRegistry &state_registry) = 0;
    virtual void print_statistics() const {
    }
    virtual ~PlanImprovement() = default;
};

#endif
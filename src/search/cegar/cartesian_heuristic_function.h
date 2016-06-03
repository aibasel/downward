#ifndef CEGAR_CARTESIAN_HEURISTIC_FUNCTION_H
#define CEGAR_CARTESIAN_HEURISTIC_FUNCTION_H

#include "refinement_hierarchy.h"

#include "../task_proxy.h"

#include <memory>

class AbstractTask;
class State;

namespace cegar {
class CartesianHeuristicFunction {
    const std::shared_ptr<AbstractTask> task;
    TaskProxy task_proxy;
    const RefinementHierarchy refinement_hierarchy;
public:
    explicit CartesianHeuristicFunction(
        const std::shared_ptr<AbstractTask> &task,
        RefinementHierarchy &&hierarchy);
    ~CartesianHeuristicFunction() = default;

    int get_value(const State &parent_state) const;
};
}

#endif

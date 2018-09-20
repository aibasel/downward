#ifndef CEGAR_CARTESIAN_HEURISTIC_FUNCTION_H
#define CEGAR_CARTESIAN_HEURISTIC_FUNCTION_H

#include "refinement_hierarchy.h"

#include "../task_proxy.h"

#include <memory>

class AbstractTask;

namespace cegar {
/*
  Store RefinementHierarchy and subtask for looking up heuristic values
  efficiently.
*/
class CartesianHeuristicFunction {
    const std::shared_ptr<AbstractTask> task;
    TaskProxy task_proxy;
    RefinementHierarchy refinement_hierarchy;

public:
    CartesianHeuristicFunction(
        const std::shared_ptr<AbstractTask> &task,
        RefinementHierarchy &&hierarchy);

    // Visual Studio 2013 needs an explicit implementation.
    CartesianHeuristicFunction(CartesianHeuristicFunction &&other)
        : task(std::move(other.task)),
          task_proxy(std::move(other.task_proxy)),
          refinement_hierarchy(std::move(other.refinement_hierarchy)) {
    }

    int get_value(const State &parent_state) const;
};
}

#endif

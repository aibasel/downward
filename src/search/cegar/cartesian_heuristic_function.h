#ifndef CEGAR_CARTESIAN_HEURISTIC_FUNCTION_H
#define CEGAR_CARTESIAN_HEURISTIC_FUNCTION_H

#include "refinement_hierarchy.h"

#include "../task_proxy.h"

#include <memory>

class AbstractTask;
class State;

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

    int get_value(const State &parent_state) const;
};
}

#endif

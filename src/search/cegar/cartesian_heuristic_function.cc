#include "cartesian_heuristic_function.h"


using namespace std;

namespace cegar {
CartesianHeuristicFunction::CartesianHeuristicFunction(
    const std::shared_ptr<AbstractTask> &task,
    RefinementHierarchy &&hierarchy)
    : task(task),
      task_proxy(*task),
      refinement_hierarchy(move(hierarchy)) {
}

int CartesianHeuristicFunction::get_value(const State &parent_state,
                                          const AbstractTask *parent_task) const {
    State local_state = task_proxy.convert_local_state(parent_state, parent_task);
    return refinement_hierarchy.get_node(local_state)->get_h_value();
}
}

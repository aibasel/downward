#include "cartesian_heuristic_function.h"

using namespace std;

namespace cegar {
CartesianHeuristicFunction::CartesianHeuristicFunction(
    const shared_ptr<AbstractTask> &task,
    const shared_ptr<RefinementHierarchy> &hierarchy)
    : task(task),
      task_proxy(*task),
      refinement_hierarchy(hierarchy) {
}

int CartesianHeuristicFunction::get_value(const State &parent_state) const {
    State local_state = task_proxy.convert_ancestor_state(parent_state);
    return refinement_hierarchy->get_node(local_state)->get_h_value();
}
}

#include "cartesian_heuristic_function.h"

#include "refinement_hierarchy.h"

#include "../utils/collections.h"

using namespace std;

namespace cartesian_abstractions {
CartesianHeuristicFunction::CartesianHeuristicFunction(
    unique_ptr<RefinementHierarchy> &&hierarchy,
    vector<int> &&h_values)
    : refinement_hierarchy(move(hierarchy)),
      h_values(move(h_values)) {
}

int CartesianHeuristicFunction::get_value(const State &state) const {
    int abstract_state_id = refinement_hierarchy->get_abstract_state_id(state);
    assert(utils::in_bounds(abstract_state_id, h_values));
    return h_values[abstract_state_id];
}
}

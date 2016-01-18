#include "cartesian_heuristic.h"

using namespace std;

namespace cegar {
CartesianHeuristic::CartesianHeuristic(RefinementHierarchy &&hierarchy)
    : refinement_hierarchy(move(hierarchy)) {
}

int CartesianHeuristic::get_value(const State &state) const {
    return refinement_hierarchy.get_node(state)->get_h_value();
}
}

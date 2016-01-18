#ifndef CEGAR_CARTESIAN_HEURISTIC_H
#define CEGAR_CARTESIAN_HEURISTIC_H

#include "refinement_hierarchy.h"

namespace cegar {
class CartesianHeuristic {
    const RefinementHierarchy refinement_hierarchy;

public:
    explicit CartesianHeuristic(RefinementHierarchy &&hierarchy);
    ~CartesianHeuristic() = default;

    int get_value(const State &state) const;
};
}

#endif

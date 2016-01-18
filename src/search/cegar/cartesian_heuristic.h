#ifndef CEGAR_CARTESIAN_HEURISTIC_H
#define CEGAR_CARTESIAN_HEURISTIC_H

#include "refinement_hierarchy.h"

#include "../heuristic.h"

class GlobalState;

namespace options {
class Options;
}

namespace cegar {
class CartesianHeuristic : public Heuristic {
    const RefinementHierarchy refinement_hierarchy;

protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &global_state);

public:
    explicit CartesianHeuristic(
        const options::Options &options, RefinementHierarchy &&hierarchy);
    ~CartesianHeuristic() = default;
};
}

#endif

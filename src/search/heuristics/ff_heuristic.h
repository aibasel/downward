#ifndef HEURISTICS_FF_HEURISTIC_H
#define HEURISTICS_FF_HEURISTIC_H

#include "additive_heuristic.h"

#include <vector>

namespace FFHeuristic {
using Proposition = RelaxationHeuristic::Proposition;
using UnaryOperator = RelaxationHeuristic::UnaryOperator;

/*
  TODO: In a better world, this should not derive from
        AdditiveHeuristic. Rather, the common parts should be
        implemented in a common base class. That refactoring could be
        made at the same time at which we also unify this with the
        other relaxation heuristics and the additional FF heuristic
        implementation in the landmark code.
*/
class FFHeuristic : public AdditiveHeuristic::AdditiveHeuristic {
    // Relaxed plans are represented as a set of operators implemented
    // as a bit vector.
    typedef std::vector<bool> RelaxedPlan;
    RelaxedPlan relaxed_plan;
    void mark_preferred_operators_and_relaxed_plan(
        const State &state, Proposition *goal);
protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &global_state);
public:
    FFHeuristic(const Options &options);
    ~FFHeuristic();
};
}

#endif

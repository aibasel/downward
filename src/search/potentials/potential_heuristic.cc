#include "potential_heuristic.h"

#include "potential_function.h"

#include "../plugins/plugin.h"

using namespace std;

namespace potentials {
PotentialHeuristic::PotentialHeuristic(
    const plugins::Options &opts, unique_ptr<PotentialFunction> function)
    : Heuristic(opts),
      function(move(function)) {
}

PotentialHeuristic::~PotentialHeuristic() {
}

int PotentialHeuristic::compute_heuristic(const State &ancestor_state) {
    State state = convert_ancestor_state(ancestor_state);
    return max(0, function->get_value(state));
}
}

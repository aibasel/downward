#include "potential_heuristic.h"

#include "potential_function.h"

#include "../plugins/plugin.h"

using namespace std;

namespace potentials {
PotentialHeuristic::PotentialHeuristic(
    const shared_ptr<AbstractTask> &task,
    unique_ptr<PotentialFunction> function, bool cache_estimates,
    const string &description, utils::Verbosity verbosity)
    : Heuristic(task, cache_estimates, description, verbosity),
      function(move(function)) {
}

int PotentialHeuristic::compute_heuristic(const State &ancestor_state) {
    State state = convert_ancestor_state(ancestor_state);
    return max(0, function->get_value(state));
}
}

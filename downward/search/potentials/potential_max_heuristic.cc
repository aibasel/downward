#include "potential_max_heuristic.h"

#include "potential_function.h"

#include "../plugins/plugin.h"

using namespace std;

namespace potentials {
PotentialMaxHeuristic::PotentialMaxHeuristic(
    vector<unique_ptr<PotentialFunction>> &&functions,
    const shared_ptr<AbstractTask> &transform, bool cache_estimates,
    const string &description, utils::Verbosity verbosity)
    : Heuristic(transform, cache_estimates, description, verbosity),
      functions(move(functions)) {
}

int PotentialMaxHeuristic::compute_heuristic(const State &ancestor_state) {
    State state = convert_ancestor_state(ancestor_state);
    int value = 0;
    for (auto &function : functions) {
        value = max(value, function->get_value(state));
    }
    return value;
}
}

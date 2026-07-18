#include "potential_max_heuristic.h"

#include "potential_function.h"

#include "../plugins/plugin.h"

using namespace std;

namespace potentials {
PotentialMaxHeuristic::PotentialMaxHeuristic(
    const shared_ptr<AbstractTask> &task,
    vector<unique_ptr<PotentialFunction>> &&functions, bool cache_estimates,
    const string &description, utils::Verbosity verbosity)
    : Heuristic(task, cache_estimates, description, verbosity),
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

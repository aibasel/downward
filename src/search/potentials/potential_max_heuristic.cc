#include "potential_max_heuristic.h"

#include "potential_function.h"

#include "../option_parser.h"

using namespace std;

namespace potentials {
PotentialMaxHeuristic::PotentialMaxHeuristic(
    const Options &opts,
    vector<unique_ptr<PotentialFunction>> &&functions)
    : Heuristic(opts),
      functions(move(functions)) {
}

int PotentialMaxHeuristic::compute_heuristic(const GlobalState &global_state) {
    const State state = convert_global_state(global_state);
    int value = 0;
    for (auto &function : functions) {
        value = max(value, function->get_value(state));
    }
    return value;
}
}

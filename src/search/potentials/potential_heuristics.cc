#include "potential_heuristics.h"

#include "potential_function.h"

#include "../option_parser.h"

using namespace std;


namespace potentials {

PotentialHeuristics::PotentialHeuristics(const Options &opts)
    : Heuristic(opts),
      functions(opts.get_list<shared_ptr<PotentialFunction> >("functions")) {
}

void PotentialHeuristics::initialize() {
}

int PotentialHeuristics::compute_heuristic(const GlobalState &global_state) {
    const State state = convert_global_state(global_state);
    int value = 0;
    for (shared_ptr<PotentialFunction> function : functions) {
        /* TODO: Can we somehow detect that one heuristic detected a dead end
           and jump out of the loop? */
        value = max(value, function->get_value(state));
    }
    return value;
}

}

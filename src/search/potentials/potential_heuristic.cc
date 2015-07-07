#include "potential_heuristic.h"

#include "potential_function.h"

#include "../option_parser.h"

using namespace std;


namespace potentials {
PotentialHeuristic::PotentialHeuristic(const Options &options)
    : Heuristic(options),
      function(options.get<shared_ptr<PotentialFunction> >("function")) {
}

int PotentialHeuristic::compute_heuristic(const GlobalState &global_state) {
    const State state = convert_global_state(global_state);
    return function->get_value(state);
}

}

#include "potential_heuristic.h"

#include "potential_function.h"

#include "../option_parser.h"

using namespace std;


namespace potentials {
PotentialHeuristic::PotentialHeuristic(
    const Options &options, shared_ptr<PotentialFunction> function)
    : Heuristic(options),
      function(function) {
}

void PotentialHeuristic::initialize() {
}

int PotentialHeuristic::compute_heuristic(const GlobalState &global_state) {
    const State state = convert_global_state(global_state);
    return function->get_value(state);
}

std::shared_ptr<Heuristic> create_potential_heuristic(
    std::shared_ptr<PotentialFunction> function) {
    Options opts;
    opts.set<int>("cost_type", NORMAL);
    return make_shared<PotentialHeuristic>(opts, function);
}

}

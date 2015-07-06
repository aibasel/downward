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

std::shared_ptr<Heuristic> create_potential_heuristic(
    std::shared_ptr<PotentialFunction> function) {
    Options opts;
    opts.set<int>("cost_type", NORMAL);
    opts.set<shared_ptr<PotentialFunction> >("function", function);
    return make_shared<PotentialHeuristic>(opts);
}
}

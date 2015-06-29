#include "potential_heuristic.h"

#include "../task_proxy.h"
#include "../utilities.h"

#include <cmath>

using namespace std;


namespace potentials {
PotentialHeuristic::PotentialHeuristic(
    const Options &options, const vector<vector<double> > &fact_potentials)
    : Heuristic(options),
      fact_potentials(fact_potentials) {
}

void PotentialHeuristic::initialize() {
}

int PotentialHeuristic::compute_heuristic(const GlobalState &global_state) {
    const State state = convert_global_state(global_state);
    double heuristic_value = 0.0;
    for (FactProxy fact : state) {
        int var_id = fact.get_variable().get_id();
        int value = fact.get_value();
        assert(in_bounds(var_id, fact_potentials));
        assert(in_bounds(value, fact_potentials[var_id]));
        heuristic_value += fact_potentials[var_id][value];
    }
    const double epsilon = 0.01;
    return static_cast<int>(max(0.0, ceil(heuristic_value - epsilon)));
}
}

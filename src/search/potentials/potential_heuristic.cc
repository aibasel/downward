#include "potential_heuristic.h"

#include "../global_state.h"
#include "../globals.h" // TODO: Remove.

#include <cmath>

using namespace std;


namespace potentials {
PotentialHeuristic::PotentialHeuristic(const Options &options,
                                       const FactPotentials &fact_potentials)
    : Heuristic(options),
      fact_potentials(fact_potentials) {
}

void PotentialHeuristic::initialize() {
}

int PotentialHeuristic::compute_heuristic(const GlobalState &state) {
    double heuristic_value = 0.0;
    int num_vars = g_variable_domain.size();
    for (int var = 0; var < num_vars; ++var) {
        heuristic_value += fact_potentials[var][state[var]];
    }
    double epsilon = 0.01;
    return static_cast<int>(max(0.0, ceil(heuristic_value - epsilon)));
}
}

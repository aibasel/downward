#include "potential_heuristics.h"

#include "../evaluation_context.h"
#include "../option_parser.h"

using namespace std;


namespace potentials {
PotentialHeuristics::PotentialHeuristics(const Options &opts)
    : Heuristic(opts),
      heuristics(opts.get_list<shared_ptr<Heuristic> >("heuristics")) {
}

void PotentialHeuristics::initialize() {
}

int PotentialHeuristics::compute_heuristic(const GlobalState &state) {
    EvaluationContext eval_context(state);
    int value = 0;
    for (shared_ptr<Heuristic> heuristic : heuristics) {
        if (eval_context.is_heuristic_infinite(heuristic.get())) {
            return DEAD_END;
        } else {
            value = max(value, eval_context.get_heuristic_value(heuristic.get()));
        }
    }
    return value;
}
}

#include "additive_cartesian_heuristic.h"

#include "cartesian_heuristic_function.h"
#include "utils.h"

#include <cassert>

using namespace std;

namespace cegar {
AdditiveCartesianHeuristic::AdditiveCartesianHeuristic(
    const options::Options &opts,
    vector<unique_ptr<CartesianHeuristicFunction>> &&heuristic_functions)
    : Heuristic(opts),
      heuristic_functions(move(heuristic_functions)) {
}

int AdditiveCartesianHeuristic::compute_heuristic(const GlobalState &global_state) {
    State state = convert_global_state(global_state);
    return compute_heuristic(state);
}

int AdditiveCartesianHeuristic::compute_heuristic(const State &state) {
    int sum_h = 0;
    for (const unique_ptr<CartesianHeuristicFunction> &func : heuristic_functions) {
        int value = func->get_value(state);
        assert(value >= 0);
        if (value == INF)
            return DEAD_END;
        sum_h += value;
    }
    assert(sum_h >= 0);
    return sum_h;
}
}

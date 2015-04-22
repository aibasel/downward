#include "cartesian_heuristic.h"

#include "utils.h"

#include "../option_parser.h"

#include <cassert>

using namespace std;

namespace cegar {
CartesianHeuristic::CartesianHeuristic(const Options &opts)
    : Heuristic(opts),
      split_tree(opts.get<SplitTree>("split_tree")) {
}

void CartesianHeuristic::initialize() {
}

int CartesianHeuristic::compute_heuristic(const GlobalState &global_state) {
    State state = task_proxy.convert_global_state(global_state);
    int h = split_tree.get_node(state)->get_h();
    assert(h >= 0);
    if (h == INF)
        return DEAD_END;
    return h;
}
}

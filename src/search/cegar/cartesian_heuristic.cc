#include "cartesian_heuristic.h"

#include "utils.h"

#include "../option_parser.h"

#include <cassert>

using namespace std;

namespace cegar {
CartesianHeuristic::CartesianHeuristic(std::shared_ptr<AbstractTask> abstract_task,
                                       const Options &opts)
    : Heuristic(opts),
      abstract_task(abstract_task),
      split_tree(opts.get<SplitTree>("split_tree")) {
}

CartesianHeuristic::~CartesianHeuristic() {
}

void CartesianHeuristic::initialize() {
}

int CartesianHeuristic::compute_heuristic(const GlobalState &global_state) {
    TaskProxy task_proxy(*abstract_task);
    State state = task_proxy.convert_global_state(global_state);
    int h = split_tree.get_node(state)->get_h();
    assert(h >= 0);
    if (h == INF)
        return DEAD_END;
    return h;
}
}

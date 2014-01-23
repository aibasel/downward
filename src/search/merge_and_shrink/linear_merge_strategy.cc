#include "linear_merge_strategy.h"

#include <cassert>

using namespace std;

LinearMergeStrategy::LinearMergeStrategy(const MergeStrategyEnum &merge_strategy_enum_)
    : MergeStrategy(merge_strategy_enum_), order(merge_strategy_enum),
      previous_index(-1) {
    // TODO: VariableOrderFinder will raise an error when choosing a non-linear
    // merge strategy, but maybe we should check this here already.
}

bool LinearMergeStrategy::done() const {
    return order.done();
}

pair<int, int> LinearMergeStrategy::get_next(const vector<Abstraction *> &all_abstractions) {
    pair<int, int> next_indices;
    if (previous_index == -1) {
        next_indices.first = order.next();
    } else {
        next_indices.first = previous_index;
    }
    next_indices.second = order.next();
    previous_index = next_indices.first;
    assert(all_abstractions[next_indices.first]);
    assert(all_abstractions[next_indices.second]);
    return next_indices;
}

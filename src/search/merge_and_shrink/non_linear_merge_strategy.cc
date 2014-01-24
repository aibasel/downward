#include "non_linear_merge_strategy.h"

#include <cstdlib>

using namespace std;

NonLinearMergeStrategy::NonLinearMergeStrategy(const MergeStrategyEnum &merge_strategy_enum_)
    : MergeStrategy(merge_strategy_enum_) {
    exit(1);
}

bool NonLinearMergeStrategy::done() const {
    exit(1);
    return true;
}

std::pair<int, int> NonLinearMergeStrategy::get_next(const std::vector<Abstraction *> &all_abstractions) {
    exit(1);
    return make_pair(all_abstractions.size(), all_abstractions.size());
}

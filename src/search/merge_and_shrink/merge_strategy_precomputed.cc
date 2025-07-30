#include "merge_strategy_precomputed.h"

#include "factored_transition_system.h"
#include "merge_tree.h"

#include <cassert>

using namespace std;

namespace merge_and_shrink {
MergeStrategyPrecomputed::MergeStrategyPrecomputed(
    const FactoredTransitionSystem &fts, unique_ptr<MergeTree> merge_tree)
    : MergeStrategy(fts), merge_tree(move(merge_tree)) {
}

pair<int, int> MergeStrategyPrecomputed::get_next() {
    assert(!merge_tree->done());
    int next_merge_index = fts.get_size();
    pair<int, int> next_merge = merge_tree->get_next_merge(next_merge_index);
    assert(fts.is_active(next_merge.first));
    assert(fts.is_active(next_merge.second));
    return next_merge;
}
}

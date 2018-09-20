#include "merge_strategy_sccs.h"

#include "factored_transition_system.h"
#include "merge_selector.h"
#include "merge_tree.h"
#include "merge_tree_factory.h"
#include "transition_system.h"

#include <algorithm>
#include <cassert>
#include <iostream>

using namespace std;

namespace merge_and_shrink {
MergeStrategySCCs::MergeStrategySCCs(
    const FactoredTransitionSystem &fts,
    const TaskProxy &task_proxy,
    const shared_ptr<MergeTreeFactory> &merge_tree_factory,
    const shared_ptr<MergeSelector> &merge_selector,
    vector<vector<int>> non_singleton_cg_sccs,
    vector<int> indices_of_merged_sccs)
    : MergeStrategy(fts),
      task_proxy(task_proxy),
      merge_tree_factory(merge_tree_factory),
      merge_selector(merge_selector),
      non_singleton_cg_sccs(move(non_singleton_cg_sccs)),
      indices_of_merged_sccs(move(indices_of_merged_sccs)),
      current_merge_tree(nullptr) {
}

MergeStrategySCCs::~MergeStrategySCCs() {
}

pair<int, int> MergeStrategySCCs::get_next() {
    // We did not already start merging an SCC/all finished SCCs, so we
    // do not have a current set of indices we want to finish merging.
    if (current_ts_indices.empty()) {
        // Get the next indices we need to merge
        if (non_singleton_cg_sccs.empty()) {
            assert(indices_of_merged_sccs.size() > 1);
            current_ts_indices = move(indices_of_merged_sccs);
        } else {
            vector<int> &current_scc = non_singleton_cg_sccs.front();
            assert(current_scc.size() > 1);
            current_ts_indices = move(current_scc);
            non_singleton_cg_sccs.erase(non_singleton_cg_sccs.begin());
        }

        // If using a merge tree factory, compute a merge tree for this set
        if (merge_tree_factory) {
            current_merge_tree = merge_tree_factory->compute_merge_tree(
                task_proxy, fts, current_ts_indices);
        }
    } else {
        // Add the most recent merge to the current indices set
        current_ts_indices.push_back(fts.get_size() - 1);
    }

    // Select the next merge for the current set of indices, either using the
    // tree or the selector.
    pair<int, int > next_pair;
    int merged_ts_index = fts.get_size();
    if (current_merge_tree) {
        assert(!current_merge_tree->done());
        next_pair = current_merge_tree->get_next_merge(merged_ts_index);
        if (current_merge_tree->done()) {
            current_merge_tree = nullptr;
        }
    } else {
        assert(merge_selector);
        next_pair = merge_selector->select_merge(fts, current_ts_indices);
    }

    // Remove the two merged indices from the current set of indices.
    for (vector<int>::iterator it = current_ts_indices.begin();
         it != current_ts_indices.end();) {
        if (*it == next_pair.first || *it == next_pair.second) {
            it = current_ts_indices.erase(it);
        } else {
            ++it;
        }
    }
    return next_pair;
}
}

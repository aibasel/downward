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
    vector<vector<int>> &&non_singleton_cg_sccs)
    : MergeStrategy(fts),
      task_proxy(task_proxy),
      merge_tree_factory(merge_tree_factory),
      merge_selector(merge_selector),
      non_singleton_cg_sccs(move(non_singleton_cg_sccs)),
      current_merge_tree(nullptr) {
}

MergeStrategySCCs::~MergeStrategySCCs() {
}

pair<int, int> MergeStrategySCCs::get_next() {
    if (current_ts_indices.empty()) {
        /*
          We are currently not dealing with merging all factors of an SCC, so
          we need to either get the next one or allow merging any existing
          factors of the FTS if there is no SCC left.
        */
        if (non_singleton_cg_sccs.empty()) {
            // We are done dealing with all SCCs, allow merging any factors.
            current_ts_indices.reserve(fts.get_num_active_entries());
            for (int ts_index: fts) {
                current_ts_indices.push_back(ts_index);
            }
        } else {
            /*
              There is another SCC we have to deal with. Store its factors so
              that we merge them over the next iterations.
            */
            vector<int> &current_scc = non_singleton_cg_sccs.front();
            assert(current_scc.size() > 1);
            current_ts_indices = move(current_scc);
            non_singleton_cg_sccs.erase(non_singleton_cg_sccs.begin());
        }

        // If using a merge tree factory, compute a merge tree for this set.
        if (merge_tree_factory) {
            current_merge_tree = merge_tree_factory->compute_merge_tree(
                task_proxy, fts, current_ts_indices);
        }
    } else {
        // Add the most recent product to the current index set.
        current_ts_indices.push_back(fts.get_size() - 1);
    }

    // Select the next merge from the current index set, either using the
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

    // Remove the two merged indices from the current index set.
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

#include "merge_strategy_sccs.h"

#include "factored_transition_system.h"
#include "merge_selector.h"
#include "transition_system.h"

#include <algorithm>
#include <cassert>
#include <iostream>

using namespace std;

namespace merge_and_shrink {
MergeStrategySCCs::MergeStrategySCCs(
    const FactoredTransitionSystem &fts,
    const shared_ptr<MergeSelector> &merge_selector,
    vector<vector<int>> &&unfinished_clusters,
    bool allow_working_on_all_clusters)
    : MergeStrategy(fts),
      merge_selector(merge_selector),
      unfinished_clusters(move(unfinished_clusters)),
      allow_working_on_all_clusters(allow_working_on_all_clusters) {
}

MergeStrategySCCs::~MergeStrategySCCs() {
}

static void compute_merge_candidates(
    const vector<int> &indices, vector<pair<int, int>> &merge_candidates) {
    for (size_t i = 0; i < indices.size(); ++i) {
        int ts_index1 = indices[i];
        for (size_t j = i + 1; j < indices.size(); ++j) {
            int ts_index2 = indices[j];
            merge_candidates.emplace_back(ts_index1, ts_index2);
        }
    }
}

pair<int, int> MergeStrategySCCs::get_next() {
    if (unfinished_clusters.empty()) {
        // We merged all clusters.
        return merge_selector->select_merge(fts);
    } else {
        // There are clusters we still have to deal with.
        vector<pair<int, int>> merge_candidates;
        vector<int> factor_to_cluster;
        if (allow_working_on_all_clusters) {
            // Compute merge candidate pairs for each cluster.
            factor_to_cluster.resize(fts.get_size(), -1);
            for (size_t cluster_index = 0;
                 cluster_index < unfinished_clusters.size(); ++cluster_index) {
                const vector<int> &cluster = unfinished_clusters[cluster_index];
                for (int factor : cluster) {
                    factor_to_cluster[factor] = cluster_index;
                }
                compute_merge_candidates(cluster, merge_candidates);
            }
        } else {
            // Deal with first cluster.
            vector<int> &cluster = unfinished_clusters.front();
            compute_merge_candidates(cluster, merge_candidates);
        }
        // Select the next merge from the allowed merge candidates.
        pair<int, int> next_pair = merge_selector->select_merge_from_candidates(
            fts, move(merge_candidates));

        // Get the cluster from which we selected the next merge.
        int affected_cluster_index;
        if (allow_working_on_all_clusters) {
            affected_cluster_index = factor_to_cluster[next_pair.first];
            assert(
                affected_cluster_index == factor_to_cluster[next_pair.second]);
        } else {
            affected_cluster_index = 0;
        }

        // Remove the two merged indices from that cluster.
        vector<int> &affected_cluster =
            unfinished_clusters[affected_cluster_index];
        for (vector<int>::iterator it = affected_cluster.begin();
             it != affected_cluster.end();) {
            if (*it == next_pair.first || *it == next_pair.second) {
                it = affected_cluster.erase(it);
            } else {
                ++it;
            }
        }

        if (affected_cluster.empty()) {
            // If the cluster got empty, remove it.
            unfinished_clusters.erase(
                unfinished_clusters.begin() + affected_cluster_index);
        } else {
            // Otherwise, add the index of the to-be-created product factor.
            affected_cluster.push_back(fts.get_size());
        }
        return next_pair;
    }
}
}

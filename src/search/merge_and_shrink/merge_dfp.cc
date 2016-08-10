#include "merge_dfp.h"

#include "distances.h"
#include "factored_transition_system.h"
#include "label_equivalence_relation.h"
#include "transition_system.h"
#include "types.h"

#include "../utils/rng.h"
#include "../utils/rng_options.h"

#include <algorithm>
#include <cassert>
#include <iostream>

using namespace std;

namespace merge_and_shrink {
MergeDFP::MergeDFP(FactoredTransitionSystem &fts, vector<int> transition_system_order)
    : MergeStrategy(fts), transition_system_order(move(transition_system_order)) {
}

bool MergeDFP::is_goal_relevant(const TransitionSystem &ts) const {
    int num_states = ts.get_size();
    for (int state = 0; state < num_states; ++state) {
        if (!ts.is_goal_state(state)) {
            return true;
        }
    }
    return false;
}

vector<int> MergeDFP::compute_label_ranks(int index) const {
    const TransitionSystem &ts = fts.get_ts(index);
    const Distances &distances = fts.get_dist(index);
    int num_labels = fts.get_num_labels();
    // Irrelevant (and inactive, i.e. reduced) labels have a dummy rank of -1
    vector<int> label_ranks(num_labels, -1);

    for (const GroupAndTransitions &gat : ts) {
        const LabelGroup &label_group = gat.label_group;
        const vector<Transition> &transitions = gat.transitions;
        // Relevant labels with no transitions have a rank of infinity.
        int label_rank = INF;
        bool group_relevant = false;
        if (static_cast<int>(transitions.size()) == ts.get_size()) {
            /*
              A label group is irrelevant in the earlier notion if it has
              exactly a self loop transition for every state.
            */
            for (const Transition &transition : transitions) {
                if (transition.target != transition.src) {
                    group_relevant = true;
                    break;
                }
            }
        } else {
            group_relevant = true;
        }
        if (!group_relevant) {
            label_rank = -1;
        } else {
            for (const Transition &transition : transitions) {
                label_rank = min(label_rank,
                                 distances.get_goal_distance(transition.target));
            }
        }
        for (int label_no : label_group) {
            label_ranks[label_no] = label_rank;
        }
    }

    return label_ranks;
}

pair<int, int> MergeDFP::compute_next_pair(
    const vector<int> &sorted_active_ts_indices) const {
    vector<bool> goal_relevant(sorted_active_ts_indices.size(), false);
    for (size_t i = 0; i < sorted_active_ts_indices.size(); ++i) {
        int ts_index = sorted_active_ts_indices[i];
        const TransitionSystem &ts = fts.get_ts(ts_index);
        if (is_goal_relevant(ts)) {
            goal_relevant[i] = true;
        }
    }

    int next_index1 = -1;
    int next_index2 = -1;
    int first_valid_pair_index1 = -1;
    int first_valid_pair_index2 = -1;
    int minimum_weight = INF;
    vector<vector<int>> transition_system_label_ranks(sorted_active_ts_indices.size());
    // Go over all pairs of transition systems and compute their weight.
    for (size_t i = 0; i < sorted_active_ts_indices.size(); ++i) {
        int ts_index1 = sorted_active_ts_indices[i];
        assert(fts.is_active(ts_index1));
        vector<int> &label_ranks1 = transition_system_label_ranks[i];
        if (label_ranks1.empty()) {
            label_ranks1 = compute_label_ranks(ts_index1);
        }
        for (size_t j = i + 1; j < sorted_active_ts_indices.size(); ++j) {
            int ts_index2 = sorted_active_ts_indices[j];
            assert(fts.is_active(ts_index2));
            if (goal_relevant[i] || goal_relevant[j]) {
                // Only consider pairs where at least one component is goal relevant.

                // Remember the first such pair
                if (first_valid_pair_index1 == -1) {
                    assert(first_valid_pair_index2 == -1);
                    first_valid_pair_index1 = ts_index1;
                    first_valid_pair_index2 = ts_index2;
                }

                // Compute the weight associated with this pair
                vector<int> &label_ranks2 = transition_system_label_ranks[j];
                if (label_ranks2.empty()) {
                    label_ranks2 = compute_label_ranks(ts_index2);
                }
                assert(label_ranks1.size() == label_ranks2.size());
                int pair_weight = INF;
                for (size_t k = 0; k < label_ranks1.size(); ++k) {
                    if (label_ranks1[k] != -1 && label_ranks2[k] != -1) {
                        // label is relevant in both transition_systems
                        int max_label_rank = max(label_ranks1[k], label_ranks2[k]);
                        pair_weight = min(pair_weight, max_label_rank);
                    }
                }
                if (pair_weight < minimum_weight) {
                    minimum_weight = pair_weight;
                    next_index1 = ts_index1;
                    next_index2 = ts_index2;
                }
            }
        }
    }

    if (next_index1 == -1) {
        /*
          No pair with finite weight has been found. In this case, we simply
          take the first pair according to our ordering consisting of at
          least one goal relevant transition system which we compute in the
          loop before. There always exists such a pair assuming that the
          global goal specification is non-empty.

          TODO: exception! with the definition of goal relevance w.r.t.
          existence of a non-goal state, there might be no such pair!
        */
        assert(next_index2 == -1);
        assert(minimum_weight == INF);
        if (first_valid_pair_index1 == -1) {
            assert(first_valid_pair_index2 == -1);
            next_index1 = sorted_active_ts_indices[0];
            next_index2 = sorted_active_ts_indices[1];
            cout << "found no goal relevant pair" << endl;
        } else {
            assert(first_valid_pair_index2 != -1);
            next_index1 = first_valid_pair_index1;
            next_index2 = first_valid_pair_index2;
        }
    }

    assert(next_index1 != -1);
    assert(next_index2 != -1);
    return make_pair(next_index1, next_index2);
}

pair<int, int> MergeDFP::get_next() {
    /*
      Precompute a vector sorted_active_ts_indices which contains all active
      transition system indices in the correct order.
    */
    assert(!transition_system_order.empty());
    vector<int> sorted_active_ts_indices;
    for (int ts_index : transition_system_order) {
        if (fts.is_active(ts_index)) {
            sorted_active_ts_indices.push_back(ts_index);
        }
    }

    pair<int, int> next_merge = compute_next_pair(sorted_active_ts_indices);
    return next_merge;
}
}

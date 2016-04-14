#include "merge_dfp.h"

#include "distances.h"
#include "factored_transition_system.h"
#include "label_equivalence_relation.h"
#include "transition_system.h"
#include "types.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_proxy.h"

#include "../utils/markup.h"

#include <algorithm>
#include <cassert>
#include <iostream>

using namespace std;

namespace merge_and_shrink {
MergeDFP::MergeDFP()
    : MergeStrategy() {
}

void MergeDFP::compute_ts_order(const shared_ptr<AbstractTask> task) {
    TaskProxy task_proxy(*task);
    int num_variables = task_proxy.get_variables().size();
    int max_transition_system_count = num_variables * 2 - 1;
    transition_system_order.reserve(max_transition_system_count);
    /*
      Precompute the order in which we consider transition systems:
      first consider the non-atomic transition systems, going from the most
      recent to the oldest one (located at index num_variables). Afterwards,
      consider the atomic transition systems in the "regular" order, i.e.
      in the Fast Downward order of variables.
    */
    for (int i = max_transition_system_count - 1; i >= 0; --i) {
        int corrected_index = i;
        if (i < num_variables) {
            corrected_index = num_variables - 1 - i;
        }
        transition_system_order.push_back(corrected_index);
    }
}

void MergeDFP::initialize(const shared_ptr<AbstractTask> task) {
    MergeStrategy::initialize(task);
    compute_ts_order(task);
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

void MergeDFP::compute_label_ranks(const FactoredTransitionSystem &fts,
                                   int index,
                                   vector<int> &label_ranks) const {
    const TransitionSystem &ts = fts.get_ts(index);
    const Distances &distances = fts.get_dist(index);
    int num_labels = fts.get_num_labels();
    // Irrelevant (and inactive, i.e. reduced) labels have a dummy rank of -1
    label_ranks.resize(num_labels, -1);

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
}

pair<int, int> MergeDFP::compute_next_pair(
    const FactoredTransitionSystem &fts,
    const vector<int> &sorted_active_ts_indices) const {
    assert(initialized());
    assert(!done());

    vector<bool> goal_relevant(sorted_active_ts_indices.size(), false);
    for (int ts_index : sorted_active_ts_indices) {
        const TransitionSystem &ts = fts.get_ts(ts_index);
        cout << ts.tag();
        if (is_goal_relevant(ts)) {
            goal_relevant[ts_index] = true;
            cout << "goal relevant" << endl;
        } else {
            cout << "not goal relevant" << endl;
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
            compute_label_ranks(fts, ts_index1, label_ranks1);
        }
        for (size_t j = i + 1; j < sorted_active_ts_indices.size(); ++j) {
            int ts_index2 = sorted_active_ts_indices[j];
            assert(fts.is_active(ts_index2));
            if (goal_relevant[ts_index1] || goal_relevant[ts_index2]) {
                // Only consider pairs where at least one component is goal relevant.

                // TODO: the 'old' code that took the 'first' pair in case of
                // no finite pair weight could be found, actually took the last
                // one, so we do the same here for the moment.
//                if (first_valid_pair_index1 == -1) {
                // Remember the first such pair
//                    assert(first_valid_pair_index2 == -1);
                first_valid_pair_index1 = ts_index1;
                first_valid_pair_index2 = ts_index2;
//                }

                // Compute the weight associated with this pair
                vector<int> &label_ranks2 = transition_system_label_ranks[j];
                if (label_ranks2.empty()) {
                    compute_label_ranks(fts, ts_index2, label_ranks2);
                };
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
          TODO: this is not correct (see above)! we take the *last* pair.
          We should eventually change this to be a random ordering.

          No pair with finite weight has been found. In this case, we simply
          take the first pair according to our ordering consisting of at
          least one goal relevant transition system which we compute in the
          loop before. There always exists such a pair assuming that the
          global goal specification is non-empty.
        */
        assert(next_index2 == -1);
        assert(minimum_weight == INF);
        assert(first_valid_pair_index1 != -1);
        assert(first_valid_pair_index2 != -1);
        next_index1 = first_valid_pair_index1;
        next_index2 = first_valid_pair_index2;
    }

    assert(next_index1 != -1);
    assert(next_index2 != -1);
    return make_pair(next_index1, next_index2);
}

pair<int, int> MergeDFP::get_next(FactoredTransitionSystem &fts) {
    assert(initialized());
    assert(!done());

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

    pair<int, int> next_merge = compute_next_pair(fts, sorted_active_ts_indices);

    --remaining_merges;
    return next_merge;
}

string MergeDFP::name() const {
    return "dfp";
}

static shared_ptr<MergeStrategy>_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Merge strategy DFP",
        "This merge strategy implements the algorithm originally described in the "
        "paper \"Directed model checking with distance-preserving abstractions\" "
        "by Draeger, Finkbeiner and Podelski (SPIN 2006), adapted to planning in "
        "the following paper:" + utils::format_paper_reference(
            {"Silvan Sievers", "Martin Wehrle", "Malte Helmert"},
            "Generalized Label Reduction for Merge-and-Shrink Heuristics",
            "http://ai.cs.unibas.ch/papers/sievers-et-al-aaai2014.pdf",
            "Proceedings of the 28th AAAI Conference on Artificial"
            " Intelligence (AAAI 2014)",
            "2358-2366",
            "AAAI Press 2014"));
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<MergeDFP>();
}

static PluginShared<MergeStrategy> _plugin("merge_dfp", _parse);
}

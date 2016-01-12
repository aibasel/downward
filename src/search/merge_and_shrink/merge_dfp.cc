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

void MergeDFP::initialize(const shared_ptr<AbstractTask> task) {
    MergeStrategy::initialize(task);
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

void MergeDFP::compute_label_ranks(FactoredTransitionSystem &fts,
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

pair<int, int> MergeDFP::get_next(FactoredTransitionSystem &fts) {
    assert(initialized());
    assert(!done());

    /*
      Precompute a vector sorted_active_ts_indices which contains all exisiting
      transition systems in the given order and compute label ranks.
    */
    assert(!transition_system_order.empty());
    vector<int> sorted_active_ts_indices;
    vector<vector<int>> transition_system_label_ranks;
    for (size_t tso_index = 0; tso_index < transition_system_order.size(); ++tso_index) {
        int ts_index = transition_system_order[tso_index];
        if (fts.is_active(ts_index)) {
            sorted_active_ts_indices.push_back(ts_index);
            transition_system_label_ranks.push_back(vector<int>());
            vector<int> &label_ranks = transition_system_label_ranks.back();
            compute_label_ranks(fts, ts_index, label_ranks);
        }
    }

    int next_index1 = -1;
    int next_index2 = -1;
    int first_valid_pair_index1 = -1;
    int first_valid_pair_index2 = -1;
    int minimum_weight = INF;
    // Go over all pairs of transition systems and compute their weight.
    for (size_t i = 0; i < sorted_active_ts_indices.size(); ++i) {
        int ts_index1 = sorted_active_ts_indices[i];
        const vector<int> &label_ranks1 = transition_system_label_ranks[i];
        assert(!label_ranks1.empty());
        for (size_t j = i + 1; j < sorted_active_ts_indices.size(); ++j) {
            int ts_index2 = sorted_active_ts_indices[j];

            if (fts.get_ts(ts_index1).is_goal_relevant()
                || fts.get_ts(ts_index2).is_goal_relevant()) {
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
                assert(!label_ranks2.empty());
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
          least one goal relevant transition system. (We computed that in the
          loop before.)
        */
        assert(next_index2 == -1);
        assert(minimum_weight == INF);
        assert(first_valid_pair_index1 != -1);
        assert(first_valid_pair_index2 != -1);
        next_index1 = first_valid_pair_index1;
        next_index2 = first_valid_pair_index2;
    }

    /*
      There always exists at least one goal relevant transition system,
      assuming that the global goal specification is non-empty. Hence at
      this point, we must have found a pair of transition systems to merge.
    */
    assert(next_index1 != -1);
    assert(next_index2 != -1);
    --remaining_merges;
    return make_pair(next_index1, next_index2);
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

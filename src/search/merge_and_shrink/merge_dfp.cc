#include "merge_dfp.h"

#include "transition_system.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../utilities.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>

using namespace std;


MergeDFP::MergeDFP()
    : MergeStrategy(),
      border_atomics_composites(remaining_merges + 1) {
    // n := remaining_merges + 1 is the number of variables of the planning task
    // and thus the number of atomic transition systems. These will be stored at
    // indices 0 to n-1 and thus n is the index at which the first composite
    // transition system will be stored at.
}

int MergeDFP::get_corrected_index(int index) const {
    // This method assumes that we iterate over the vector of all
    // transition systems in inverted order (from back to front). It returns the
    // unmodified index as long as we are in the range of composite
    // transition systems (these are thus traversed in order from the last one
    // to the first one) and modifies the index otherwise so that the order
    // in which atomic transition systems are considered is from the first to the
    // last one (from front to back). This is to emulate the previous behavior
    // when new transition systems were not inserted after existing transition systems,
    // but rather replaced arbitrarily one of the two original transition systems.
    assert(index >= 0);
    if (index >= border_atomics_composites)
        return index;
    return border_atomics_composites - 1 - index;
}

void MergeDFP::compute_label_ranks(const TransitionSystem *transition_system,
                                   vector<int> &label_ranks) const {
    int num_labels = transition_system->get_num_labels();
    // Irrelevant (and inactive, i.e. reduced) labels have a dummy rank of -1
    label_ranks.resize(num_labels, -1);

    const std::list<LabelGroup> &grouped_labels = transition_system->get_grouped_labels();
    for (LabelGroupConstIter group_it = grouped_labels.begin();
         group_it != grouped_labels.end(); ++group_it) {
        // Relevant labels with no transitions have a rank of infinity.
        int label_rank = INF;
        const vector<Transition> &transitions = group_it->get_const_transitions();
        bool group_relevant = false;
        if (static_cast<int>(transitions.size()) == transition_system->get_size()) {
            /*
              A label group is irrelevant in the earlier notion if it has
              exactly a self loop transition for every state.
            */
            for (size_t i = 0; i < transitions.size(); ++i) {
                if (transitions[i].target != transitions[i].src) {
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
            for (size_t i = 0; i < transitions.size(); ++i) {
                const Transition &t = transitions[i];
                label_rank = min(label_rank, transition_system->get_goal_distance(t.target));
            }
        }
        for (LabelConstIter label_it = group_it->begin();
             label_it != group_it->end(); ++label_it) {
            int label_no = *label_it;
            label_ranks[label_no] = label_rank;
        }
    }
}

pair<int, int> MergeDFP::get_next(const vector<TransitionSystem *> &all_transition_systems) {
    assert(!done());

    vector<const TransitionSystem *> sorted_transition_systems;
    vector<int> indices_mapping;
    vector<vector<int> > transition_system_label_ranks;
    // Precompute a vector sorted_transition_systems which contains all exisiting
    // transition systems from all_transition_systems in the desired order.
    for (int i = all_transition_systems.size() - 1; i >= 0; --i) {
        // We iterate from back to front, considering the composite
        // transition systems in the order from "most recently added" (= at the back
        // of the vector) to "first added" (= at border_atomics_composites).
        // Afterwards, we consider the atomic transition systems in the "regular"
        // order from the first one until the last one. See also explanation
        // at get_corrected_index().
        int ts_index = get_corrected_index(i);
        const TransitionSystem *transition_system = all_transition_systems[ts_index];
        if (transition_system) {
            sorted_transition_systems.push_back(transition_system);
            indices_mapping.push_back(ts_index);
            transition_system_label_ranks.push_back(vector<int>());
            vector<int> &label_ranks = transition_system_label_ranks[transition_system_label_ranks.size() - 1];
            compute_label_ranks(transition_system, label_ranks);
        }
    }

    int first = -1;
    int second = -1;
    int minimum_weight = INF;
    for (size_t ts_index = 0; ts_index < sorted_transition_systems.size(); ++ts_index) {
        const TransitionSystem *transition_system = sorted_transition_systems[ts_index];
        assert(transition_system);
        vector<int> &label_ranks = transition_system_label_ranks[ts_index];
        assert(!label_ranks.empty());
        for (size_t other_ts_index = ts_index + 1; other_ts_index < sorted_transition_systems.size();
             ++other_ts_index) {
            const TransitionSystem *other_transition_system = sorted_transition_systems[other_ts_index];
            assert(other_transition_system);
            if (transition_system->is_goal_relevant() || other_transition_system->is_goal_relevant()) {
                vector<int> &other_label_ranks = transition_system_label_ranks[other_ts_index];
                assert(!other_label_ranks.empty());
                assert(label_ranks.size() == other_label_ranks.size());
                int pair_weight = INF;
                for (size_t i = 0; i < label_ranks.size(); ++i) {
                    if (label_ranks[i] != -1 && other_label_ranks[i] != -1) {
                        // label is relevant in both transition_systems
                        int max_label_rank = max(label_ranks[i], other_label_ranks[i]);
                        pair_weight = min(pair_weight, max_label_rank);
                    }
                }
                if (pair_weight < minimum_weight) {
                    minimum_weight = pair_weight;
                    first = indices_mapping[ts_index];
                    second = indices_mapping[other_ts_index];
                    assert(all_transition_systems[first] == transition_system);
                    assert(all_transition_systems[second] == other_transition_system);
                }
            }
        }
    }
    if (first == -1) {
        // No pair with finite weight has been found. In this case, we simply
        // take the first pair according to our ordering consisting of at
        // least one goal relevant transition system.
        assert(second == -1);
        assert(minimum_weight == INF);

        for (size_t ts_index = 0; ts_index < sorted_transition_systems.size(); ++ts_index) {
            const TransitionSystem *transition_system = sorted_transition_systems[ts_index];
            assert(transition_system);
            for (size_t other_ts_index = ts_index + 1; other_ts_index < sorted_transition_systems.size();
                 ++other_ts_index) {
                const TransitionSystem *other_transition_system = sorted_transition_systems[other_ts_index];
                assert(other_transition_system);
                if (transition_system->is_goal_relevant() || other_transition_system->is_goal_relevant()) {
                    first = indices_mapping[ts_index];
                    second = indices_mapping[other_ts_index];
                    assert(all_transition_systems[first] == transition_system);
                    assert(all_transition_systems[second] == other_transition_system);
                }
            }
        }
    }
    /*
      There always exists at least one goal relevant transition system,
      assuming that the global goal specification is non-empty. Hence at
      this point, we must have found a pair of transition systems to merge.
    */
    assert(first != -1);
    assert(second != -1);
    cout << "Next pair of indices: (" << first << ", " << second << ")" << endl;
//    if (remaining_merges > 1 && minimum_weight != INF) {
//        // in the case we do not make a trivial choice of a next pair
//        cout << "Computed weight: " << minimum_weight << endl;
//    } else {
//        cout << "No weight computed (pair has been chosen trivially by order)" << endl;
//    }
    --remaining_merges;
    return make_pair(first, second);
}

string MergeDFP::name() const {
    return "dfp";
}

static MergeStrategy *_parse(OptionParser &parser) {
    if (parser.dry_run())
        return 0;
    else
        return new MergeDFP();
}

static Plugin<MergeStrategy> _plugin("merge_dfp", _parse);

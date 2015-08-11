#include "merge_dfp.h"

#include "transition_system.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <algorithm>
#include <cassert>
#include <iostream>

using namespace std;


MergeDFP::MergeDFP()
    : MergeStrategy() {
}

void MergeDFP::initialize(const shared_ptr<AbstractTask> task) {
    MergeStrategy::initialize(task);
    /*
      n := remaining_merges + 1 is the number of variables of the planning task
      and thus the number of atomic transition systems. These will be stored at
      indices 0 to n-1 and thus n is the index at which the first composite
      transition system will be stored at.
    */
    border_atomics_composites = remaining_merges + 1;
}

int MergeDFP::get_corrected_index(int index) const {
    /*
      This method assumes that we iterate over the vector of all
      transition systems in inverted order (from back to front). It returns the
      unmodified index as long as we are in the range of composite
      transition systems (these are thus traversed in order from the last one
      to the first one) and modifies the index otherwise so that the order
      in which atomic transition systems are considered is from the first to the
      last one (from front to back). This is to emulate the previous behavior
      when new transition systems were not inserted after existing transition systems,
      but rather replaced arbitrarily one of the two original transition systems.
    */
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

    for (TSConstIterator group_it = transition_system->begin();
         group_it != transition_system->end(); ++group_it) {
        // Relevant labels with no transitions have a rank of infinity.
        int label_rank = INF;
        const vector<Transition> &transitions = group_it.get_transitions();
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
        for (LabelConstIter label_it = group_it.begin();
             label_it != group_it.end(); ++label_it) {
            int label_no = *label_it;
            label_ranks[label_no] = label_rank;
        }
    }
}

pair<int, int> MergeDFP::get_next(const vector<TransitionSystem *> &all_transition_systems) {
    assert(initialized());
    assert(!done());

    vector<const TransitionSystem *> sorted_transition_systems;
    vector<int> indices_mapping;
    vector<vector<int> > transition_system_label_ranks;
    /*
      Precompute a vector sorted_transition_systems which contains all exisiting
      transition systems from all_transition_systems in the desired order and
      compute label ranks.
    */
    for (int i = all_transition_systems.size() - 1; i >= 0; --i) {
        /*
          We iterate from back to front, considering the composite
          transition systems in the order from "most recently added" (= at the back
          of the vector) to "first added" (= at border_atomics_composites).
          Afterwards, we consider the atomic transition systems in the "regular"
          order from the first one until the last one. See also explanation
          at get_corrected_index().
        */
        int ts_index = get_corrected_index(i);
        const TransitionSystem *transition_system = all_transition_systems[ts_index];
        if (transition_system) {
            sorted_transition_systems.push_back(transition_system);
            indices_mapping.push_back(ts_index);
            transition_system_label_ranks.push_back(vector<int>());
            vector<int> &label_ranks = transition_system_label_ranks.back();
            compute_label_ranks(transition_system, label_ranks);
        }
    }

    int next_index1 = -1;
    int next_index2 = -1;
    int minimum_weight = INF;
    for (size_t i = 0; i < sorted_transition_systems.size(); ++i) {
        const TransitionSystem *transition_system1 = sorted_transition_systems[i];
        assert(transition_system1);
        const vector<int> &label_ranks1 = transition_system_label_ranks[i];
        assert(!label_ranks1.empty());
        for (size_t j = i + 1; j < sorted_transition_systems.size(); ++j) {
            const TransitionSystem *transition_system2 = sorted_transition_systems[j];
            assert(transition_system2);
            if (transition_system1->is_goal_relevant()
                || transition_system2->is_goal_relevant()) {
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
                    next_index1 = indices_mapping[i];
                    next_index2 = indices_mapping[j];
                    assert(all_transition_systems[next_index1] == transition_system1);
                    assert(all_transition_systems[next_index2] == transition_system2);
                }
            }
        }
    }
    if (next_index1 == -1) {
        /*
          No pair with finite weight has been found. In this case, we simply
          take the first pair according to our ordering consisting of at
          least one goal relevant transition system.
        */
        assert(next_index2 == -1);
        assert(minimum_weight == INF);

        for (size_t i = 0; i < sorted_transition_systems.size(); ++i) {
            const TransitionSystem *transition_system1 = sorted_transition_systems[i];
            for (size_t j = i + 1; j < sorted_transition_systems.size(); ++j) {
                const TransitionSystem *transition_system2 = sorted_transition_systems[j];
                if (transition_system1->is_goal_relevant()
                    || transition_system2->is_goal_relevant()) {
                    next_index1 = indices_mapping[i];
                    next_index2 = indices_mapping[j];
                    assert(all_transition_systems[next_index1] == transition_system1);
                    assert(all_transition_systems[next_index2] == transition_system2);
                }
            }
        }
    }
    /*
      There always exists at least one goal relevant transition system,
      assuming that the global goal specification is non-empty. Hence at
      this point, we must have found a pair of transition systems to merge.
    */
    assert(next_index1 != -1);
    assert(next_index2 != -1);
    cout << "Next pair of indices: (" << next_index1 << ", " << next_index2 << ")" << endl;
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
        "the following paper:\n\n"
        " * Silvan Sievers, Martin Wehrle, and Malte Helmert.<<BR>>\n"
        " [Generalized Label Reduction for Merge-and-Shrink Heuristics "
        "http://ai.cs.unibas.ch/papers/sievers-et-al-aaai2014.pdf].<<BR>>\n "
        "In //Proceedings of the 28th AAAI Conference on Artificial "
        "Intelligence (AAAI 2014)//, pp. 2358-2366. AAAI Press 2014.");
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<MergeDFP>();
}

static PluginShared<MergeStrategy> _plugin("merge_dfp", _parse);

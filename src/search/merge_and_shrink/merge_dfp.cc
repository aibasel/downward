#include "merge_dfp.h"

#include "abstraction.h"
#include "label.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../utilities.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>

using namespace std;

//  TODO: We define infinity in more than a few places right now (=>
//        grep for it). It should only be defined once.
static const int infinity = numeric_limits<int>::max();

MergeDFP::MergeDFP()
    : MergeStrategy(), remaining_merges(-1) {
}

bool MergeDFP::done() const {
    return remaining_merges == 0;
}

pair<int, int> MergeDFP::get_next(const std::vector<Abstraction *> &all_abstractions) {
    if (remaining_merges == -1) {
        remaining_merges = all_abstractions.size() - 1;
        indices_order.reserve(all_abstractions.size());
        for (size_t i = 0; i < all_abstractions.size(); ++i) {
            indices_order.push_back(i);
        }
    }
    assert(remaining_merges > 0);

    // Precompute a vector sorted_abstrations which contains all exisiting
    // abstractions from all_abstractions in the desired order.
    vector<Abstraction *> sorted_abstractions;
    // sorted_abs_to_indices_order maps abstraction indices as stored in
    // sorted_abstractions to the corresponding index of indices_order. This
    // index then serves to find out the index of the abstraction in
    // all_abstractions.
    vector<int> sorted_abs_to_indices_order;
    vector<vector<int> > abstraction_label_ranks;
    for (size_t i = 0; i < indices_order.size(); ++i) {
        size_t abs_index = indices_order[i];
        if (abs_index != infinity) {
            Abstraction *abstraction = all_abstractions[abs_index];
            assert(abstraction);
            sorted_abstractions.push_back(abstraction);
            sorted_abs_to_indices_order.push_back(i);
            abstraction_label_ranks.push_back(vector<int>());
            vector<int> &label_ranks = abstraction_label_ranks[abstraction_label_ranks.size() - 1];
            abstraction->compute_label_ranks(label_ranks);
        }
    }

    int first = -1;
    int second = -1;
    int minimum_weight = infinity;
    for (size_t abs_index = 0; abs_index < sorted_abstractions.size(); ++abs_index) {
        Abstraction *abstraction = sorted_abstractions[abs_index];
        assert(abstraction);
        vector<int> &label_ranks = abstraction_label_ranks[abs_index];
        assert(!label_ranks.empty());
        for (size_t other_abs_index = abs_index + 1; other_abs_index < sorted_abstractions.size();
             ++other_abs_index) {
            Abstraction *other_abstraction = sorted_abstractions[other_abs_index];
            assert(other_abstraction);
            if (abstraction->is_goal_relevant() || other_abstraction->is_goal_relevant()) {
                vector<int> &other_label_ranks = abstraction_label_ranks[other_abs_index];
                assert(!other_label_ranks.empty());
                assert(label_ranks.size() == other_label_ranks.size());
                int pair_weight = infinity;
                for (size_t i = 0; i < label_ranks.size(); ++i) {
                    if (label_ranks[i] != -1 && other_label_ranks[i] != -1) {
                        // label is relevant in both abstractions
                        int max_label_rank = max(label_ranks[i], other_label_ranks[i]);
                        pair_weight = min(pair_weight, max_label_rank);
                    }
                }
                if (pair_weight < minimum_weight) {
                    minimum_weight = pair_weight;
                    first = sorted_abs_to_indices_order[abs_index];
                    second = sorted_abs_to_indices_order[other_abs_index];
                    assert(all_abstractions[indices_order[first]] == abstraction);
                    assert(all_abstractions[indices_order[second]] == other_abstraction);
                }
            }
        }
    }
    if (first == -1) {
        // No pair with finite weight has been found. In this case, we simply
        // take the first pair according to our ordering consisting of at
        // least one goal relevant abstraction.
        assert(second == -1);
        assert(minimum_weight == infinity);

        for (size_t abs_index = 0; abs_index < sorted_abstractions.size(); ++abs_index) {
            Abstraction *abstraction = sorted_abstractions[abs_index];
            assert(abstraction);
            for (size_t other_abs_index = abs_index + 1; other_abs_index < sorted_abstractions.size();
                 ++other_abs_index) {
                Abstraction *other_abstraction = sorted_abstractions[other_abs_index];
                assert(other_abstraction);
                if (abstraction->is_goal_relevant() || other_abstraction->is_goal_relevant()) {
                    first = sorted_abs_to_indices_order[abs_index];
                    second = sorted_abs_to_indices_order[other_abs_index];
                    assert(all_abstractions[indices_order[first]] == abstraction);
                    assert(all_abstractions[indices_order[second]] == other_abstraction);
                }
            }
        }
    }
    assert(first != -1);
    assert(second != -1);
    int f = first;
    int s = second;
    first = indices_order[first];
    second = indices_order[second];
    cout << "Next pair of indices: (" << first << ", " << second << ")" << endl;
//    if (remaining_merges > 1 && minimum_weight != infinity) {
//        // in the case we do not make a trivial choice of a next pair
//        cout << "Computed weight: " << minimum_weight << endl;
//    } else {
//        cout << "No weight computed (pair has been chosen trivially by order)" << endl;
//    }
    --remaining_merges;
    indices_order[f] = all_abstractions.size();
    indices_order[s] = infinity;
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

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

MergeDFP::MergeDFP(const Options &opts)
    : MergeStrategy(),
      abstraction_order(AbstractionOrder(opts.get_enum("abstraction_order"))),
      remaining_merges(-1),
      border_atomics_composites(-1) {
}

bool MergeDFP::done() const {
    return remaining_merges == 0;
}

size_t MergeDFP::get_corrected_index(int index) const {
    // This method assumes that we iterate over the vector of all
    // abstractions in inverted order (from back to front). It returns the
    // unmodified index as long as we are in the range of composite
    // abstractions (these are thus traversed in order from the last one
    // to the first one) and modifies the index otherwise so that the order
    // in which atomic abstractions are considered is from the first to the
    // last one (from front to back). This is to emulate the previous behavior
    // when new abstractions were not inserted after existing abstractions,
    // but rather replaced arbitrarily one of the two original abstractions.
    assert(index >= 0);
    if (index >= border_atomics_composites)
        return index;
    return border_atomics_composites - 1 - index;
}

pair<int, int> MergeDFP::get_next(const std::vector<Abstraction *> &all_abstractions) {
    if (remaining_merges == -1) {
        remaining_merges = all_abstractions.size() - 1;
        if (abstraction_order == ALL_COMPOSITES_THEN_ATOMICS) {
            border_atomics_composites = all_abstractions.size();
        } else if (abstraction_order == EMULATE_PREVIOUS) {
            indices_order.reserve(all_abstractions.size());
            for (size_t i = 0; i < all_abstractions.size(); ++i) {
                indices_order.push_back(i);
            }
        }
    }
    assert(remaining_merges > 0);

    vector<Abstraction *> sorted_abstractions;
    vector<int> indices_mapping;
    vector<vector<int> > abstraction_label_ranks;
    if (abstraction_order == ALL_COMPOSITES_THEN_ATOMICS) {
        // Precompute a vector sorted_abstrations which contains all exisiting
        // abstractions from all_abstractions in the desired order.
        for (int i = all_abstractions.size() - 1; i >= 0; --i) {
            // We iterate from back to front, considering the composite
            // abstractions in the order from "most recently added" (= at the back
            // of the vector) to "first added" (= at border_atomics_composites).
            // Afterwards, we consider the atomic abstrations in the "regular"
            // order from the first one until the last one. See also explanation
            // at get_corrected_index().
            size_t abs_index = get_corrected_index(i);
            Abstraction *abstraction = all_abstractions[abs_index];
            if (abstraction) {
                sorted_abstractions.push_back(abstraction);
                indices_mapping.push_back(abs_index);
                abstraction_label_ranks.push_back(vector<int>());
                vector<int> &label_ranks = abstraction_label_ranks[abstraction_label_ranks.size() - 1];
                abstraction->compute_label_ranks(label_ranks);
            }
        }
    } else if (abstraction_order == EMULATE_PREVIOUS) {
        // Precompute a vector sorted_abstrations which contains all exisiting
        // abstractions from all_abstractions in the desired order.

        // indices_mapping maps abstraction indices as stored in
        // sorted_abstractions to the corresponding index of indices_order. This
        // index then serves to find out the index of the abstraction in
        // all_abstractions.
        for (size_t i = 0; i < indices_order.size(); ++i) {
            size_t abs_index = indices_order[i];
            if (abs_index != infinity) {
                Abstraction *abstraction = all_abstractions[abs_index];
                assert(abstraction);
                sorted_abstractions.push_back(abstraction);
                indices_mapping.push_back(i);
                abstraction_label_ranks.push_back(vector<int>());
                vector<int> &label_ranks = abstraction_label_ranks[abstraction_label_ranks.size() - 1];
                abstraction->compute_label_ranks(label_ranks);
            }
        }
    }

    int first = -1;
    int second = -1;
    int minimum_weight = infinity;
    bool first_abs_goal_relevant = false;
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
                    first = indices_mapping[abs_index];
                    second = indices_mapping[other_abs_index];
                    if (abstraction->is_goal_relevant())
                        first_abs_goal_relevant = true;
                    if (abstraction_order == ALL_COMPOSITES_THEN_ATOMICS) {
                        assert(all_abstractions[first] == abstraction);
                        assert(all_abstractions[second] == other_abstraction);
                    } else if (abstraction_order == EMULATE_PREVIOUS) {
                        assert(all_abstractions[indices_order[first]] == abstraction);
                        assert(all_abstractions[indices_order[second]] == other_abstraction);
                    }
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
                    first = indices_mapping[abs_index];
                    second = indices_mapping[other_abs_index];
                    if (abstraction->is_goal_relevant())
                        first_abs_goal_relevant = true;
                    if (abstraction_order == ALL_COMPOSITES_THEN_ATOMICS) {
                        assert(all_abstractions[first] == abstraction);
                        assert(all_abstractions[second] == other_abstraction);
                    } else if (abstraction_order == EMULATE_PREVIOUS) {
                        assert(all_abstractions[indices_order[first]] == abstraction);
                        assert(all_abstractions[indices_order[second]] == other_abstraction);
                    }
                }
            }
        }
    }
    assert(first != -1);
    assert(second != -1);
    int f = first;
    int s = second;
    if (abstraction_order == EMULATE_PREVIOUS) {
        first = indices_order[first];
        second = indices_order[second];
    }
    cout << "Next pair of indices: (" << first << ", " << second << ")" << endl;
//    if (remaining_merges > 1 && minimum_weight != infinity) {
//        // in the case we do not make a trivial choice of a next pair
//        cout << "Computed weight: " << minimum_weight << endl;
//    } else {
//        cout << "No weight computed (pair has been chosen trivially by order)" << endl;
//    }
    --remaining_merges;
    if (abstraction_order == EMULATE_PREVIOUS) {
        if (first_abs_goal_relevant) {
            indices_order[f] = all_abstractions.size();
            indices_order[s] = infinity;
        } else {
            indices_order[f] = infinity;
            indices_order[s] = all_abstractions.size();
        }
    }
    return make_pair(first, second);
}

string MergeDFP::name() const {
    return "dfp";
}

static MergeStrategy *_parse(OptionParser &parser) {
    vector<string> abstraction_order;
    abstraction_order.push_back("all_composites_then_atomics");
    abstraction_order.push_back("emulate_previous");
    parser.add_enum_option("abstraction_order", abstraction_order,
                           "order in which dfp considers abstractions "
                           "(important for tie breaking",
                           "all_composites_then_atomics");
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new MergeDFP(opts);
}

static Plugin<MergeStrategy> _plugin("merge_dfp", _parse);

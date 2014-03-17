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
    : MergeStrategy(),
      remaining_merges(-1) {
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
    // last one (from front to back).
    assert(index >= 0);
    if (index >= border_atomics_composites)
        return index;
    return border_atomics_composites - 1 - index;
}

pair<int, int> MergeDFP::get_next(const std::vector<Abstraction *> &all_abstractions) {
    /* Note: if we just invert the regular order (i.e. go through
       all_abstractions from the last to the first element), we obtain very
       bad results (h=10 on tpp06). Intermediate solutions are:
       - use the inverted order only in the outer loop
       - *always* start with the last element and then iterate through
         all_abstractions in the regular order (h=25 on tpp06)
       - in the first iteration, use the regular forward traversal, and from
         the second on, start with the last element
       - as previous method, but try to return the "bigger" abstraction as
         first index as to avoid unnecessary shrinking.
     */
    if (remaining_merges == -1) {
        remaining_merges = all_abstractions.size() - 1;
        border_atomics_composites = all_abstractions.size();
    }
    int first = -1;
    int second = -1;
    int minimum_weight = infinity;
    if (remaining_merges > 1) {
        vector<vector<int> > abstraction_label_ranks(all_abstractions.size());
        // We iterate from back to front, considering the composite
        // abstractions in the order from "most recently added" (= at the back
        // of the vector) to "first added" (= at border_atomics_composites).
        // Afterwards, we consider the atomic abstrations in the "regular"
        // order from the first one until the last one. See also explanation
        // at get_corrected_index().
        for (int i = all_abstractions.size() - 1; i >= 0; --i) {
            size_t abs_index = get_corrected_index(i);
            Abstraction *abstraction = all_abstractions[abs_index];
            if (abstraction) {
                vector<int> &label_ranks = abstraction_label_ranks[abs_index];
                if (label_ranks.empty()) {
                    abstraction->compute_label_ranks(label_ranks);
                }
                for (int j = i - 1; j >= 0; --j) {
                    size_t other_abs_index = get_corrected_index(j);
                    assert (other_abs_index != abs_index);
                    Abstraction *other_abstraction = all_abstractions[other_abs_index];
                    if (other_abstraction) {
                        if (!abstraction->is_goal_relevant() && !other_abstraction->is_goal_relevant()) {
                            // only consider pairs where at least one abstraction is goal relevant
                            continue;
                        }
                        vector<int> &other_label_ranks = abstraction_label_ranks[other_abs_index];
                        if (other_label_ranks.empty()) {
                            other_abstraction->compute_label_ranks(other_label_ranks);
                        }

                        assert(label_ranks.size() == other_label_ranks.size());
                        int pair_weight = infinity;
                        for (size_t i = 0; i < label_ranks.size(); ++i) {
                            if (label_ranks[i] != -1 && other_label_ranks[i] != -1) {
                                int max_label_rank = max(label_ranks[i], other_label_ranks[i]);
                                pair_weight = min(pair_weight, max_label_rank);
                            }
                        }
                        if (pair_weight < minimum_weight) {
                            minimum_weight = pair_weight;
                            // always return a goal relevant abstraction as a first index
                            if (abstraction->is_goal_relevant()) {
                                first = abs_index;
                                second = other_abs_index;
                            } else {
                                assert(other_abstraction->is_goal_relevant());
                                first = other_abs_index;
                                second = abs_index;
                            }
                        }
                    }
                }
            }
        }
    }
    if (first == -1) {
        assert(second == -1);
        assert(remaining_merges == 1 || minimum_weight == infinity);
        // we simply take the first two valid indices from the set of all
        // abstractions (prefering goal relevant abstractions) to be merged next if:
        // 1) remaining_merges = 1 (there are only two abstractions left)
        // 2) all pair weights have been computed to be infinity
        for (size_t abs_index = 0; abs_index < all_abstractions.size(); ++abs_index) {
            Abstraction *abstraction = all_abstractions[abs_index];
            if (abstraction) {
                for (size_t other_abs_index = abs_index + 1; other_abs_index < all_abstractions.size(); ++other_abs_index) {
                    Abstraction *other_abstraction = all_abstractions[other_abs_index];
                    if (other_abstraction) {
                        if (!abstraction->is_goal_relevant() && !other_abstraction->is_goal_relevant()) {
                            // only consider pairs where at least one abstraction is goal relevant
                            continue;
                        }
                        // always return a goal relevant abstraction as a first index
                        if (abstraction->is_goal_relevant()) {
                            first = abs_index;
                            second = other_abs_index;
                        } else {
                            assert(other_abstraction->is_goal_relevant());
                            first = other_abs_index;
                            second = abs_index;
                        }
                    }
                }
            }
        }
    }
    assert(first != -1);
    assert(second != -1);
    cout << "Next pair of indices: (" << first << ", " << second << ")" << endl;
//    if (remaining_merges > 1 && minimum_weight != infinity) {
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
    if (!parser.dry_run())
        return new MergeDFP();
    else
        return 0;
}

static Plugin<MergeStrategy> _plugin("merge_dfp", _parse);

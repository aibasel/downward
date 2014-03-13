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

pair<int, int> MergeDFP::get_next(const std::vector<Abstraction *> &all_abstractions) {
    if (remaining_merges == -1) {
        remaining_merges = all_abstractions.size() - 1;
    }
    int first = -1;
    int second = -1;
    int minimum_weight = infinity;
    if (remaining_merges > 1) {
        vector<vector<int> > abstraction_label_ranks(all_abstractions.size());
        for (size_t abs_index = 0; abs_index < all_abstractions.size(); ++abs_index) {
            Abstraction *abstraction = all_abstractions[abs_index];
            if (abstraction) {
                vector<int> &label_ranks = abstraction_label_ranks[abs_index];
                if (label_ranks.empty()) {
                    abstraction->compute_label_ranks(label_ranks);
                }
                for (size_t other_abs_index = abs_index + 1; other_abs_index < all_abstractions.size(); ++other_abs_index) {
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
        // 2) when all pair weights have been computed to be infinity
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

#include "non_linear_merge_strategy.h"

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

NonLinearMergeStrategy::NonLinearMergeStrategy(const Options &opts)
    : MergeStrategy(),
      non_linear_merge_strategy_type(NonLinearMergeStrategyType(opts.get_enum("type"))),
      remaining_merges(-1) {
}

bool NonLinearMergeStrategy::done() const {
    return remaining_merges == 0;
}

pair<int, int> NonLinearMergeStrategy::get_next(const std::vector<Abstraction *> &all_abstractions) {
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
    int start_index;
    //int max_index;
    if (remaining_merges == -1) {
        remaining_merges = all_abstractions.size() - 1;
        start_index = 0;
        //max_index = all_abstractions.size();
    } else {
        start_index = -1;
        //max_index = all_abstractions.size() - 1;
    }
    int first = -1;
    int second = -1;
    int minimum_weight = infinity;
    if (remaining_merges > 1) {
        vector<vector<int> > abstraction_label_ranks(all_abstractions.size());

        if (start_index != 0) {
            size_t abs1 = all_abstractions.size() - 1;
            Abstraction *abst1 = all_abstractions[abs1];
            assert(abst1->is_goal_relevant());
            size_t abs2 = 0;
            Abstraction *abst2 = all_abstractions[abs2];
            while (!abst2) {
                ++abs2;
                abst2 = all_abstractions[abs2];
            }
            vector<int> &label_ranks1 = abstraction_label_ranks[abs1];
            abst1->compute_label_ranks(label_ranks1);
            vector<int> &label_ranks2 = abstraction_label_ranks[abs2];
            abst2->compute_label_ranks(label_ranks2);
            int pair_weight = infinity;
            for (size_t i = 0; i < label_ranks1.size(); ++i) {
                if (label_ranks1[i] != -1 && label_ranks2[i] != -1) {
                    int max_label_rank = max(label_ranks1[i], label_ranks2[i]);
                    pair_weight = min(pair_weight, max_label_rank);
                }
            }
            cout << abs1 << " " << abs2 << " " << pair_weight << endl;
            minimum_weight = pair_weight;
            // always return a goal relevant abstraction as a first index
            first = abs1;
            second = abs2;
        }

        //for (int i = -1; i < all_abstractions.size() - 1; ++i) {
        for (int i = 0; i < all_abstractions.size(); ++i) {
            size_t abs_index = i;
//            if (i == -1)
//                abs_index = all_abstractions.size() - 1;
//            else
//                abs_index = i;
            Abstraction *abstraction = all_abstractions[abs_index];
            if (abstraction) {
                vector<int> &label_ranks = abstraction_label_ranks[abs_index];
                if (label_ranks.empty()) {
                    abstraction->compute_label_ranks(label_ranks);
                }
                for (size_t j = i + 1; j < all_abstractions.size(); ++j) {
                    size_t other_abs_index = j;
                    if (other_abs_index == abs_index)
                        continue;
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
                        //cout << abs_index << " " << other_abs_index << " " << pair_weight << endl;
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

void NonLinearMergeStrategy::dump_strategy_specific_options() const {
    cout << "Non linear merge strategy type: ";
    switch (non_linear_merge_strategy_type) {
    case DFP:
        cout << "DFP";
        break;
    default:
        ABORT("Unknown merge strategy.");
    }
    cout << endl;
}

string NonLinearMergeStrategy::name() const {
    return "non linear";
}

static MergeStrategy *_parse(OptionParser &parser) {
    vector<string> merge_strategies;
    //TODO: it's a bit dangerous that the merge strategies here
    // have to be specified exactly in the same order
    // as in the enum definition. Try to find a way around this,
    // or at least raise an error when the order is wrong.
    merge_strategies.push_back("DFP");
    parser.add_enum_option("type", merge_strategies,
                           "non linear merge strategy",
                           "DFP");

    Options opts = parser.parse();
    if (parser.help_mode())
        return 0;
    if (!parser.dry_run())
        return new NonLinearMergeStrategy(opts);
    else
        return 0;
}

static Plugin<MergeStrategy> _plugin("merge_non_linear", _parse);

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
      remaining_merges(-1), pair_weights_unequal_zero_counter(0) {
}

bool NonLinearMergeStrategy::done() const {
    if (remaining_merges == 0) {
        cout << "Number of times a pair with weight unequal 0 has been chosen: "
             << pair_weights_unequal_zero_counter << endl;
    }
    return remaining_merges == 0;
}

void NonLinearMergeStrategy::get_next(const std::vector<Abstraction *> &all_abstractions, pair<int, int> &next_indices) {
    if (remaining_merges == -1) {
        remaining_merges = all_abstractions.size() - 1;
    }
    if (remaining_merges == 1) {
        for (size_t abs_index = 0; abs_index < all_abstractions.size(); ++abs_index) {
            if (all_abstractions[abs_index]) {
                for (size_t other_abs_index = abs_index + 1; other_abs_index < all_abstractions.size(); ++other_abs_index) {
                    if (all_abstractions[other_abs_index]) {
                        next_indices.first = abs_index;
                        next_indices.second = other_abs_index;
                        --remaining_merges;
                        return;
                    }
                }
            }
        }
    }
    //next_indices.first = -1;
    //next_indices.second = -1;
    vector<vector<int> > abstraction_label_ranks(all_abstractions.size());
    int minimum_weight = infinity;
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
                    vector<int> &other_label_ranks = abstraction_label_ranks[other_abs_index];
                    if (other_label_ranks.empty()) {
                        other_abstraction->compute_label_ranks(other_label_ranks);
                    }

                    //cout << "comparing label ranks for " << abs_index << " and " << other_abs_index << endl,
                    assert(label_ranks.size() == other_label_ranks.size());
                    int pair_weight = infinity;
                    for (size_t i = 0; i < label_ranks.size(); ++i) {
                        if (label_ranks[i] != -1 && other_label_ranks[i] != -1) {
                            //cout << "label " << i << ": " << label_ranks[i] << " " << other_label_ranks[i] << endl;
                            int max_label_rank = max(label_ranks[i], other_label_ranks[i]);
                            pair_weight = min(pair_weight, max_label_rank);
                        }
                    }
                    //if (pair_weight != 0 && pair_weight != infinity) {
                    //    cout << "pair weight for " << abs_index << " and " << other_abs_index << ": " << pair_weight << endl;
                    //}
                    if (pair_weight < minimum_weight/* && pair_weight > 0*/) {
                        minimum_weight = pair_weight;
                        next_indices.first = abs_index;
                        next_indices.second = other_abs_index;
                    }
                }
            }
        }
    }
    /*int index = 0;
    while (next_indices.second == -1) {
        Abstraction *abs = all_abstractions[index];
        if (abs) {
            if (next_indices.first == -1) {
                next_indices.first = index;
                ++index;
                continue;
            }
            if (next_indices.second == -1) {
                assert(next_indices.first != -1);
                next_indices.second = index;
                break;
            }
        } else {
            ++index;
        }
    }*/
    if (minimum_weight != 0 ) {
        ++pair_weights_unequal_zero_counter;
    }
    --remaining_merges;
}

void NonLinearMergeStrategy::dump_strategy_specific_options() const {
    cout << "Non linear merge strategy type: ";
    switch (non_linear_merge_strategy_type) {
    case MERGE_NON_LINEAR_DFP:
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
    merge_strategies.push_back("MERGE_NON_LINEAR_DFP");
    parser.add_enum_option("type", merge_strategies,
                           "non linear merge strategy",
                           "MERGE_NON_LINEAR_DFP");

    Options opts = parser.parse();
    if (parser.help_mode())
        return 0;
    if (!parser.dry_run())
        return new NonLinearMergeStrategy(opts);
    else
        return 0;
}

static Plugin<MergeStrategy> _plugin("merge_non_linear", _parse);

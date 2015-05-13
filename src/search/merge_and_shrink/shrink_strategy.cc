#include "shrink_strategy.h"

#include "transition_system.h"

#include "../option_parser.h"
#include "../utilities.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace std;


ShrinkStrategy::ShrinkStrategy(const Options &opts)
    : max_states(opts.get<int>("max_states")),
      max_states_before_merge(opts.get<int>("max_states_before_merge")),
      shrink_threshold_before_merge(opts.get<int>("threshold")) {
    assert(max_states_before_merge > 0);
    assert(max_states >= max_states_before_merge);
    assert(shrink_threshold_before_merge <= max_states_before_merge);
}

ShrinkStrategy::~ShrinkStrategy() {
}

/*
  TODO: I think we could get a nicer division of responsibilities if
  this method were part of the transition system class. The shrink
  strategies would then return generate an equivalence class
  ("collapsed_groups") and not modify the transition system, which would be
  passed as const.
 */
void ShrinkStrategy::apply(TransitionSystem &ts,
    const StateEquivalenceRelation &equivalence_relation,
    int target) const {
    // TODO: We currently violate this; see issue250
    //assert(equivalence_relation.size() <= target);
    ts.apply_abstraction(equivalence_relation);
    cout << ts.tag() << "size after shrink " << ts.get_size()
         << ", target " << target << endl;
    //assert(ts.size() <= target);
}

bool ShrinkStrategy::must_shrink(
    const TransitionSystem &ts, int threshold) const {
    assert(threshold >= 1);
    assert(ts.is_solvable());
    int num_states = ts.get_size();
    if (num_states > threshold) {
        cout << ts.tag() << "shrink from size " << num_states
             << " (threshold: " << threshold << ")" << endl;
        return true;
    }
    return false;
}

pair<size_t, size_t> ShrinkStrategy::compute_shrink_sizes(
    size_t size1, size_t size2) const {
    // Bound both sizes by max allowed size before merge.
    size_t max_before_merge = max_states_before_merge;
    size_t new_size1 = min(size1, max_before_merge);
    size_t new_size2 = min(size2, max_before_merge);

    if (!is_product_within_limit(new_size1, new_size2, max_states)) {
        size_t balanced_size = size_t(sqrt(max_states));

        if (new_size1 <= balanced_size) {
            // Size of the first transition system is small enough. Use whatever
            // is left for the second transition system.
            new_size2 = max_states / new_size1;
        } else if (new_size2 <= balanced_size) {
            // Inverted case as before.
            new_size1 = max_states / new_size2;
        } else {
            // Both transition systems are too big. We set both target sizes
            // to balanced_size. An alternative would be to set one to
            // N1 = balanced_size and the other to N2 = max_states /
            // balanced_size, to get closer to the allowed maximum.
            // However, this would make little difference (N2 would
            // always be N1, N1 + 1 or N1 + 2), and our solution has the
            // advantage of treating the transition systems symmetrically.
            new_size1 = balanced_size;
            new_size2 = balanced_size;
        }
    }
    assert(new_size1 <= size1 && new_size2 <= size2);
    assert(static_cast<int>(new_size1) <= max_states_before_merge);
    assert(static_cast<int>(new_size2) <= max_states_before_merge);
    assert(static_cast<int>(new_size1 * new_size2) <= max_states);
    return make_pair(new_size1, new_size2);
}

void ShrinkStrategy::shrink_before_merge(TransitionSystem &ts1, TransitionSystem &ts2) {
    pair<int, int> new_sizes = compute_shrink_sizes(
        ts1.get_size(), ts2.get_size());
    int new_size1 = new_sizes.first;
    int new_size2 = new_sizes.second;

    /*
      Shrink one or both transition systems if their size is larger than
      the size limit imposed by max_states and max_states_before_merge,
      or if their size is larger than threshold. In the former case, the
      transition system is forced to be shrunk down to the size limit, in
      the latter case, the shrink strategy is still called with the new
      size limit, which means that it does not have to shrink the size of the
      transition system, but it *may* do so. This is useful e.g. for the
      bisimulation shrink strategy, which searches for a perfect bisimulation
      (which is information-preserving) as long as the size limit is not
      exceeded.
    */

    if (must_shrink(ts2, min(new_size2, shrink_threshold_before_merge))) {
        StateEquivalenceRelation equivalence_relation;
        shrink(ts2, new_size2, equivalence_relation);
        apply(ts2, equivalence_relation, new_size2);
    }

    if (must_shrink(ts1, min(new_size1, shrink_threshold_before_merge))) {
        StateEquivalenceRelation equivalence_relation;
        shrink(ts1, new_size1, equivalence_relation);
        apply(ts1, equivalence_relation, new_size1);
    }
}

void ShrinkStrategy::dump_options() const {
    cout << "Shrink strategy options: " << endl;
    cout << "Transition system size limit: " << max_states << endl
         << "Transition system size limit right before merge: "
         << max_states_before_merge << endl;
    cout << "Threshold: " << shrink_threshold_before_merge << endl;
    cout << "Type: " << name() << endl;
    dump_strategy_specific_options();
}

string ShrinkStrategy::get_name() const {
    return name();
}

void ShrinkStrategy::add_options_to_parser(OptionParser &parser) {
    parser.add_option<int>(
        "max_states",
        "maximum transition system size allowed at any time point.",
        "-1");
    parser.add_option<int>(
        "max_states_before_merge",
        "maximum transition system size allowed for two transition systems "
        "before being merged to form the synchronized product.",
        "-1");
    parser.add_option<int>(
        "threshold",
        "If a transition system, before being merged, surpasses this soft "
        "transition system size limit, the shrink strategy is called to "
        "possibly shrink the transition system.",
        "-1");
}

void ShrinkStrategy::handle_option_defaults(Options &opts) {
    int max_states = opts.get<int>("max_states");
    int max_states_before_merge = opts.get<int>("max_states_before_merge");
    int threshold = opts.get<int>("threshold");

    // If none of the two state limits has been set: set default limit.
    if (max_states == -1 && max_states_before_merge == -1) {
        max_states = 50000;
    }

    // If exactly one of the max_states options has been set, set the other
    // so that it imposes no further limits.
    if (max_states_before_merge == -1) {
        max_states_before_merge = max_states;
    } else if (max_states == -1) {
        int n = max_states_before_merge;
        if (is_product_within_limit(n, n, numeric_limits<int>::max())) {
            max_states = n * n;
        } else {
            max_states = numeric_limits<int>::max();
        }
    }

    if (max_states_before_merge > max_states) {
        cerr << "warning: max_states_before_merge exceeds max_states, "
             << "correcting." << endl;
        max_states_before_merge = max_states;
    }

    if (max_states < 1) {
        cerr << "error: transition system size must be at least 1" << endl;
        exit_with(EXIT_INPUT_ERROR);
    }

    if (max_states_before_merge < 1) {
        cerr << "error: transition system size before merge must be at least 1"
             << endl;
        exit_with(EXIT_INPUT_ERROR);
    }

    if (threshold == -1) {
        threshold = max_states;
    }
    if (threshold < 1) {
        cerr << "error: threshold must be at least 1" << endl;
        exit_with(EXIT_INPUT_ERROR);
    }
    if (threshold > max_states) {
        cerr << "warning: threshold exceeds max_states, correcting" << endl;
        threshold = max_states;
    }

    opts.set<int>("max_states", max_states);
    opts.set<int>("max_states_before_merge", max_states_before_merge);
    opts.set<int>("threshold", threshold);
}

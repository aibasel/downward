#include "merge_scoring_function_miasm_utils.h"

#include "distances.h"
#include "factored_transition_system.h"
#include "shrink_strategy.h"
#include "transition_system.h"
#include "utils.h"

#include "../utils/logging.h"

#include <algorithm>

using namespace std;

namespace merge_and_shrink {
/*
  Compute a state equivalence relation for the given transition system with
  the given shrink strategy, respecting the given size limit new_size. If the
  result of applying it actually reduced the size of the transition system,
  copy the transition system, apply the state equivalence relation to it and
  return the result. Return nullptr otherwise.
*/
static unique_ptr<TransitionSystem> copy_and_shrink_ts(
    const TransitionSystem &ts,
    const Distances &distances,
    const ShrinkStrategy &shrink_strategy,
    int new_size,
    utils::LogProxy &log) {
    /*
      TODO: think about factoring out common logic of this function and the
      function shrink_factor in utils.cc
    */
    StateEquivalenceRelation equivalence_relation =
        shrink_strategy.compute_equivalence_relation(ts, distances, new_size, log);
    // TODO: We currently violate this; see issue250
    //assert(equivalence_relation.size() <= target_size);
    int new_num_states = equivalence_relation.size();

    if (new_num_states < ts.get_size()) {
        /*
          If we actually shrink the transition system, we first need to copy it,
          then shrink it and return it.
        */
        vector<int> abstraction_mapping = compute_abstraction_mapping(
            ts.get_size(), equivalence_relation);
        unique_ptr<TransitionSystem> ts_copy =
            make_unique<TransitionSystem>(ts);
        ts_copy->apply_abstraction(
            equivalence_relation, abstraction_mapping, log);
        return ts_copy;
    } else {
        return nullptr;
    }
}

unique_ptr<TransitionSystem> shrink_before_merge_externally(
    const FactoredTransitionSystem &fts,
    int index1,
    int index2,
    const ShrinkStrategy &shrink_strategy,
    int max_states,
    int max_states_before_merge,
    int shrink_threshold_before_merge,
    utils::LogProxy &log) {
    const TransitionSystem &original_ts1 = fts.get_transition_system(index1);
    const TransitionSystem &original_ts2 = fts.get_transition_system(index2);

    /*
      Determine size limits and if shrinking is necessary or possible as done
      in the merge-and-shrink loop.
    */
    pair<int, int> new_sizes = compute_shrink_sizes(
        original_ts1.get_size(),
        original_ts2.get_size(),
        max_states_before_merge,
        max_states);
    bool must_shrink_ts1 = original_ts1.get_size() > min(new_sizes.first, shrink_threshold_before_merge);
    bool must_shrink_ts2 = original_ts2.get_size() > min(new_sizes.second, shrink_threshold_before_merge);

    /*
      If we need to shrink, copy_and_shrink_ts will take care of computing
      a copy, shrinking it, and returning it. (In cases where shrinking is
      only triggered due to the threshold being passed but no perfect
      shrinking is possible, the method returns a null pointer.)
    */
    unique_ptr<TransitionSystem> ts1 = nullptr;
    if (must_shrink_ts1) {
        ts1 = copy_and_shrink_ts(
            original_ts1,
            fts.get_distances(index1),
            shrink_strategy,
            new_sizes.first,
            log);
    }
    unique_ptr<TransitionSystem> ts2 = nullptr;
    if (must_shrink_ts2) {
        ts2 = copy_and_shrink_ts(
            original_ts2,
            fts.get_distances(index2),
            shrink_strategy,
            new_sizes.second,
            log);
    }

    /*
      Return the product, using either the original transition systems or
      the copied and shrunk ones.
    */
    return TransitionSystem::merge(
        fts.get_labels(),
        (ts1 ? *ts1 : original_ts1),
        (ts2 ? *ts2 : original_ts2),
        log);
}
}

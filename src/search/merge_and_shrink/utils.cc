#include "utils.h"

#include "distances.h"
#include "factored_transition_system.h"
#include "shrink_strategy.h"
#include "transition_system.h"

#include "../utils/math.h"
#include "../utils/memory.h"

#include <algorithm>
#include <cassert>
#include <cmath>

using namespace std;

namespace merge_and_shrink {
/*
  Compute target sizes for shrinking two transition systems with sizes size1
  and size2 before they are merged. Use the following rules:
  1) Right before merging, the transition systems may have at most
     max_states_before_merge states.
  2) Right after merging, the product has at most max_states_after_merge states.
  3) Transition systems are shrunk as little as necessary to satisfy the above
     constraints. (If possible, neither is shrunk at all.)
  There is often a Pareto frontier of solutions following these rules. In this
  case, balanced solutions (where the target sizes are close to each other)
  are preferred over less balanced ones.
*/
pair<int, int> compute_shrink_sizes(
    int size1,
    int size2,
    int max_states_before_merge,
    int max_states_after_merge) {
    // Bound both sizes by max allowed size before merge.
    int new_size1 = min(size1, max_states_before_merge);
    int new_size2 = min(size2, max_states_before_merge);

    if (!utils::is_product_within_limit(
            new_size1, new_size2, max_states_after_merge)) {
        int balanced_size = int(sqrt(max_states_after_merge));

        if (new_size1 <= balanced_size) {
            // Size of the first transition system is small enough. Use whatever
            // is left for the second transition system.
            new_size2 = max_states_after_merge / new_size1;
        } else if (new_size2 <= balanced_size) {
            // Inverted case as before.
            new_size1 = max_states_after_merge / new_size2;
        } else {
            // Both transition systems are too big. We set both target sizes
            // to balanced_size. An alternative would be to set one to
            // N1 = balanced_size and the other to N2 = max_states_after_merge /
            // balanced_size, to get closer to the allowed maximum.
            // However, this would make little difference (N2 would
            // always be N1, N1 + 1 or N1 + 2), and our solution has the
            // advantage of treating the transition systems symmetrically.
            new_size1 = balanced_size;
            new_size2 = balanced_size;
        }
    }
    assert(new_size1 <= size1 && new_size2 <= size2);
    assert(new_size1 <= max_states_before_merge);
    assert(new_size2 <= max_states_before_merge);
    assert(new_size1 * new_size2 <= max_states_after_merge);
    return make_pair(new_size1, new_size2);
}

/*
  This method checks if the transition system of the factor at index violates
  the size limit given via new_size (e.g. as computed by compute_shrink_sizes)
  or the threshold shrink_threshold_before_merge that triggers shrinking even
  if the size limit is not violated. If so, trigger the shrinking process.
  Return true iff the factor was actually shrunk.
*/
bool shrink_factor(
    FactoredTransitionSystem &fts,
    int index,
    int new_size,
    int shrink_threshold_before_merge,
    const ShrinkStrategy &shrink_strategy,
    Verbosity verbosity) {
    const TransitionSystem &ts = fts.get_ts(index);
    int num_states = ts.get_size();
    if (num_states > min(new_size, shrink_threshold_before_merge)) {
        if (verbosity >= Verbosity::VERBOSE) {
            cout << ts.tag() << "current size: " << num_states;
            if (new_size < num_states)
                cout << " (new size limit: " << new_size;
            else
                cout << " (shrink threshold: " << shrink_threshold_before_merge;
            cout << ")" << endl;
        }

        const Distances &distances = fts.get_distances(index);
        StateEquivalenceRelation equivalence_relation =
            shrink_strategy.compute_equivalence_relation(ts, distances, new_size);
        // TODO: We currently violate this; see issue250
        //assert(equivalence_relation.size() <= target_size);
        return fts.apply_abstraction(index, equivalence_relation, verbosity);
    }
    return false;
}

bool shrink_before_merge_step(
    FactoredTransitionSystem &fts,
    int index1,
    int index2,
    int max_states,
    int max_states_before_merge,
    int shrink_threshold_before_merge,
    const ShrinkStrategy &shrink_strategy,
    Verbosity verbosity) {
    /*
      Compute the size limit for both transition systems as imposed by
      max_states and max_states_before_merge.
    */
    pair<int, int> new_sizes = compute_shrink_sizes(
        fts.get_ts(index1).get_size(),
        fts.get_ts(index2).get_size(),
        max_states_before_merge,
        max_states);

    /*
      For both transition systems, possibly compute and apply an
      abstraction.
      TODO: we could better use the given limit by increasing the size limit
      for the second shrinking if the first shrinking was larger than
      required.
    */
    bool shrunk1 = shrink_factor(
        fts,
        index1,
        new_sizes.first,
        shrink_threshold_before_merge,
        shrink_strategy,
        verbosity);
    if (verbosity >= Verbosity::VERBOSE && shrunk1) {
        fts.statistics(index1);
    }
    bool shrunk2 = shrink_factor(
        fts,
        index2,
        new_sizes.second,
        shrink_threshold_before_merge,
        shrink_strategy,
        verbosity);
    if (verbosity >= Verbosity::VERBOSE && shrunk2) {
        fts.statistics(index2);
    }
    return shrunk1 || shrunk2;
}

bool prune_step(
    FactoredTransitionSystem &fts,
    int index,
    bool prune_unreachable_states,
    bool prune_irrelevant_states,
    Verbosity verbosity) {
    assert(prune_unreachable_states || prune_irrelevant_states);
    const TransitionSystem &ts = fts.get_ts(index);
    const Distances &distances = fts.get_distances(index);
    int num_states = ts.get_size();
    StateEquivalenceRelation state_equivalence_relation;
    state_equivalence_relation.reserve(num_states);
    int unreachable_count = 0;
    int irrelevant_count = 0;
    int dead_count = 0;
    for (int state = 0; state < num_states; ++state) {
        /* If pruning both unreachable and irrelevant states, a state which is
           dead is counted for both statistics! */
        bool prune_state = false;
        if (prune_unreachable_states && distances.get_init_distance(state) == INF) {
            ++unreachable_count;
            prune_state = true;
        }
        if (prune_irrelevant_states && distances.get_goal_distance(state) == INF) {
            ++irrelevant_count;
            prune_state = true;
        }
        if (prune_state) {
            ++dead_count;
        } else {
            StateEquivalenceClass state_equivalence_class;
            state_equivalence_class.push_front(state);
            state_equivalence_relation.push_back(state_equivalence_class);
        }
    }
    if (verbosity >= Verbosity::VERBOSE &&
        (unreachable_count || irrelevant_count)) {
        cout << ts.tag()
             << "unreachable: " << unreachable_count << " states, "
             << "irrelevant: " << irrelevant_count << " states ("
             << "total dead: " << dead_count << " states)" << endl;
    }
    return fts.apply_abstraction(index, state_equivalence_relation, verbosity);
}

vector<int> compute_abstraction_mapping(
    int num_states,
    const StateEquivalenceRelation &equivalence_relation) {
    vector<int> abstraction_mapping(num_states, PRUNED_STATE);
    for (size_t class_no = 0; class_no < equivalence_relation.size(); ++class_no) {
        const StateEquivalenceClass &state_equivalence_class =
            equivalence_relation[class_no];
        for (int state : state_equivalence_class) {
            assert(abstraction_mapping[state] == PRUNED_STATE);
            abstraction_mapping[state] = class_no;
        }
    }
    return abstraction_mapping;
}

bool is_goal_relevant(const TransitionSystem &ts) {
    int num_states = ts.get_size();
    for (int state = 0; state < num_states; ++state) {
        if (!ts.is_goal_state(state)) {
            return true;
        }
    }
    return false;
}

/*
  Compute a state equivalence relation for the given transition system with
  the given shrink strategy, respecting the given size limit new_size. If the
  result of applying it actually reduced the size of the transition system,
  copy the transition system, apply the state equivalence relation to it and
  return the result. Return nullptr otherwise.
*/
unique_ptr<TransitionSystem> copy_and_shrink_ts(
    const TransitionSystem &ts,
    const Distances &distances,
    const ShrinkStrategy &shrink_strategy,
    int new_size,
    Verbosity verbosity) {
    StateEquivalenceRelation equivalence_relation =
        shrink_strategy.compute_equivalence_relation(ts, distances, new_size);
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
            utils::make_unique_ptr<TransitionSystem>(ts);
        ts_copy->apply_abstraction(
            equivalence_relation, abstraction_mapping, verbosity);
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
    int shrink_threshold_before_merge) {
    const TransitionSystem &original_ts1 = fts.get_ts(index1);
    const TransitionSystem &original_ts2 = fts.get_ts(index2);

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
    Verbosity verbosity = Verbosity::SILENT;
    unique_ptr<TransitionSystem> ts1 = nullptr;
    if (must_shrink_ts1) {
        ts1 = copy_and_shrink_ts(
            original_ts1,
            fts.get_distances(index1),
            shrink_strategy,
            new_sizes.first,
            verbosity);
    }
    unique_ptr<TransitionSystem> ts2 = nullptr;
    if (must_shrink_ts2) {
        ts2 = copy_and_shrink_ts(
            original_ts2,
            fts.get_distances(index2),
            shrink_strategy,
            new_sizes.second,
            verbosity);
    }

    /*
      Return the product, using either the original transition systems or
      the copied and shrunk ones.
    */
    return TransitionSystem::merge(
        fts.get_labels(),
        (ts1 ? *ts1 : original_ts1),
        (ts2 ? *ts2: original_ts2),
        verbosity);
}
}

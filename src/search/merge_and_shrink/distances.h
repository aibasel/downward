#ifndef MERGE_AND_SHRINK_DISTANCES_H
#define MERGE_AND_SHRINK_DISTANCES_H

#include "types.h"

#include <vector>

/*
  TODO: Possible interface improvements for this class:
  - Check TODOs in implementation file.

  (Many of these would need performance tests, as distance computation
  can be one of the bottlenecks in our code.)

  TODO: Possible performance improvements:
  - We currently try to keep the distance information after shrinking
    (going quite far to avoid recomputation). Does this really serve a
    purpose? *After* shrinking, why are we interested in the distances
    in the first place? The main point of the distances is to inform
    the shrink strategies. (OK, I guess some merge strategies care
    about them, too -- but should *all* merge strategies pay for that?
    And if these merge strategies only care about h values, which we
    usually preserve, wouldn't it be better to invalidate g values
    then?)
*/

namespace merge_and_shrink {
class TransitionSystem;

class Distances {
    static const int DISTANCE_UNKNOWN = -1;

    const TransitionSystem &transition_system;

    std::vector<int> init_distances;
    std::vector<int> goal_distances;

    void clear_distances();
    int get_num_states() const;
    bool is_unit_cost() const;

    void compute_init_distances_unit_cost();
    void compute_goal_distances_unit_cost();
    void compute_init_distances_general_cost();
    void compute_goal_distances_general_cost();
public:
    explicit Distances(const TransitionSystem &transition_system);
    ~Distances();

    bool are_distances_computed() const;
    void compute_distances(
        bool compute_init_distances,
        bool compute_goal_distances,
        Verbosity verbosity);

    /*
      Update distances according to the given abstraction. If the abstraction
      is not f-preserving, distances are directly recomputed.

      It is OK for the abstraction to drop states, but then all
      dropped states must be unreachable or irrelevant. (Otherwise,
      the method might fail to detect that the distance information is
      out of date.)
    */
    void apply_abstraction(
        const StateEquivalenceRelation &state_equivalence_relation,
        Verbosity verbosity,
        bool compute_init_distances,
        bool compute_goal_distances);

    int get_init_distance(int state) const {
        return init_distances[state];
    }

    int get_goal_distance(int state) const {
        return goal_distances[state];
    }

    void dump() const;
    void statistics() const;
};
}

#endif

#ifndef MERGE_AND_SHRINK_DISTANCES_H
#define MERGE_AND_SHRINK_DISTANCES_H

class TransitionSystem;

#include <forward_list>
#include <vector>


/*
  TODO: Possible interface improvements for this class:

  - Get rid of please_let_me_mess_with_... methods.
  - Rather than have compute_distances() return a vector<bool>,
    store it inside as an attribute (just like init_distances and
    goal_distances).
  - Check TODOs in implementation file.

  (Many of these would need performance tests, as distance computation
  can be one of the bottlenecks in our code.)

  TODO: Possible performance improvements:
  - Make the interface more fine-grained, so that it becomes possible
    to only compute the things that we actually want to use. For
    example, in many configurations we do not really care about g
    values (although we might still want to prune unreachable
    states... we might want to make this configurable).
  - Get rid of the vector<bool> of prunable states; this can be
    deduced from init_distances and goal_distances anyway. Not clear
    which of these options would be better.
  - We currently try to keep the distance information after shrinking
    (going quite far to avoid recomputation; see the caller of the
    please_let_me_mess_with_... methods). Does this really serve a
    purpose? *After* shrinking, why are we interested in the distances
    in the first place? The main point of the distances is to inform
    the shrink strategies. (OK, I guess some merge strategies care
    about them, too -- but should *all* merge strategies pay for that?
    And if these merge strategies only care about h values, which we
    usually preserve, wouldn't it be better to invalidate g values
    then?)
*/

class Distances {
    static const int DISTANCE_UNKNOWN = -1;

    const TransitionSystem &transition_system;

    std::vector<int> init_distances;
    std::vector<int> goal_distances;

    int max_f;
    int max_g;
    int max_h;

    int get_num_states() const;
    bool is_unit_cost() const;

    void compute_init_distances_unit_cost();
    void compute_goal_distances_unit_cost();
    void compute_init_distances_general_cost();
    void compute_goal_distances_general_cost();
public:
    explicit Distances(const TransitionSystem &transition_system);
    ~Distances();

    void clear_distances();
    bool are_distances_computed() const;
    std::vector<bool> compute_distances();

    /*
      Update distances according to the given abstraction.
      Returns true iff the abstraction was f-preserving.

      It is OK for the abstraction to drop states, but then all
      dropped states must be unreachable or irrelevant. (Otherwise,
      the method might fail to detect that the distance information is
      out of date.)
    */
    bool apply_abstraction(
        const std::vector<std::forward_list<int> > &collapsed_groups);

    int get_max_f() const;
    int get_max_g() const;
    int get_max_h() const;

    int get_init_distance(int state) const {
        return init_distances[state];
    }

    int get_goal_distance(int state) const {
        return goal_distances[state];
    }
};

#endif

#ifndef MERGE_AND_SHRINK_DISTANCES_H
#define MERGE_AND_SHRINK_DISTANCES_H

class TransitionSystem;

#include <vector>

class Distances {
    /*
      TODO: Remove the following friend. We only need it while we're
      refactoring to make sure the "old" and "new" distance
      computations are in sync.
    */
    friend class TransitionSystem;

    // TODO: The following two are copied from transition_system.h;
    // that's just a temporary hack.
    static const int PRUNED_STATE = -1;
    static const int DISTANCE_UNKNOWN = -2;

    const TransitionSystem &transition_system;

    std::vector<int> init_distances;
    std::vector<int> goal_distances;

    int max_f;
    int max_g;
    int max_h;

    int get_num_states() const;
public:
    explicit Distances(const TransitionSystem &transition_system);
    ~Distances();

    void clear_distances();
    void compute_init_distances_unit_cost();
    void compute_goal_distances_unit_cost();
    void compute_init_distances_general_cost();
    void compute_goal_distances_general_cost();
    bool are_distances_computed() const;
    bool is_unit_cost() const;
    std::vector<bool> compute_distances();

    int get_max_f() const {
        return max_f;
    }

    int get_max_g() const {
        return max_g;
    }

    int get_max_h() const {
        return max_h;
    }

    int get_init_distance(int state) const {
        return init_distances[state];
    }

    int get_goal_distance(int state) const {
        return goal_distances[state];
    }
};

#endif

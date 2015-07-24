#ifndef MERGE_AND_SHRINK_DISTANCES_H
#define MERGE_AND_SHRINK_DISTANCES_H

class TransitionSystem;

#include <vector>


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

    int get_max_f() const;
    int get_max_g() const;
    int get_max_h() const;

    int get_init_distance(int state) const {
        return init_distances[state];
    }

    int get_goal_distance(int state) const {
        return goal_distances[state];
    }

    /*
      TODO: The following two methods are temporary and should
      eventually be removed. See comments in
      TransitionSystem::apply_abstraction().
    */
    std::vector<int> &please_let_me_mess_with_init_distances() {
        return init_distances;
    }

    std::vector<int> &please_let_me_mess_with_goal_distances() {
        return goal_distances;
    }
};

#endif

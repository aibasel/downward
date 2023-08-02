#ifndef CARTESIAN_ABSTRACTIONS_ABSTRACT_SEARCH_H
#define CARTESIAN_ABSTRACTIONS_ABSTRACT_SEARCH_H

#include "transition.h"
#include "types.h"

#include "../algorithms/priority_queues.h"

#include <deque>
#include <memory>
#include <vector>

namespace cartesian_abstractions {
using Solution = std::deque<Transition>;

/*
  Find abstract solutions using A*.
*/
class AbstractSearch {
    class AbstractSearchInfo {
        int g;
        int h;
        Transition incoming_transition;
public:
        AbstractSearchInfo()
            : h(0),
              incoming_transition(UNDEFINED, UNDEFINED) {
            reset();
        }

        void reset() {
            g = std::numeric_limits<int>::max();
            incoming_transition = Transition(UNDEFINED, UNDEFINED);
        }

        void decrease_g_value_to(int new_g) {
            assert(new_g <= g);
            g = new_g;
        }

        int get_g_value() const {
            return g;
        }

        void increase_h_value_to(int new_h) {
            assert(new_h >= h);
            h = new_h;
        }

        int get_h_value() const {
            return h;
        }

        void set_incoming_transition(const Transition &transition) {
            incoming_transition = transition;
        }

        const Transition &get_incoming_transition() const {
            assert(incoming_transition.op_id != UNDEFINED &&
                   incoming_transition.target_id != UNDEFINED);
            return incoming_transition;
        }
    };

    const std::vector<int> operator_costs;

    // Keep data structures around to avoid reallocating them.
    priority_queues::AdaptiveQueue<int> open_queue;
    std::vector<AbstractSearchInfo> search_info;

    void reset(int num_states);
    void set_h_value(int state_id, int h);
    std::unique_ptr<Solution> extract_solution(int init_id, int goal_id) const;
    void update_goal_distances(const Solution &solution);
    int astar_search(
        const std::vector<Transitions> &transitions,
        const Goals &goals);

public:
    explicit AbstractSearch(const std::vector<int> &operator_costs);

    std::unique_ptr<Solution> find_solution(
        const std::vector<Transitions> &transitions,
        int init_id,
        const Goals &goal_ids);
    int get_h_value(int state_id) const;
    void copy_h_value_to_children(int v, int v1, int v2);
};

std::vector<int> compute_distances(
    const std::vector<Transitions> &transitions,
    const std::vector<int> &costs,
    const std::unordered_set<int> &start_ids);
}

#endif

#ifndef LANDMARKS_LANDMARK_H
#define LANDMARKS_LANDMARK_H

#include "../task_proxy.h"

#include <set>

namespace landmarks {
class Landmark {
public:
    Landmark(std::vector<FactPair> _facts, bool disjunctive, bool conjunctive,
             bool is_true_in_goal = false, bool is_derived = false)
        : facts(move(_facts)), disjunctive(disjunctive), conjunctive(conjunctive),
          is_true_in_goal(is_true_in_goal), is_derived(is_derived), cost(1) {
        assert(!(conjunctive && disjunctive));
        assert((conjunctive && facts.size() > 1)
               || (disjunctive && facts.size() > 1) || facts.size() == 1);
    }

    bool operator ==(const Landmark &other) const {
        return this == &other;
    }

    bool operator !=(const Landmark &other) const {
        return !(*this == other);
    }

    std::vector<FactPair> facts;
    bool disjunctive;
    bool conjunctive;
    bool is_true_in_goal;
    bool is_derived;

    // Cost of achieving the landmark (as determined by the landmark factory)
    int cost;

    std::set<int> first_achievers;
    std::set<int> possible_achievers;

    bool is_true_in_state(const State &state) const;
};
}
#endif

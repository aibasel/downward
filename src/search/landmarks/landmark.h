#ifndef LANDMARKS_LANDMARK_H
#define LANDMARKS_LANDMARK_H

#include "../task_proxy.h"

#include <unordered_set>

namespace landmarks {
class Landmark {
public:
    Landmark(std::vector<FactPair> _atoms, bool is_disjunctive,
             bool is_conjunctive, bool is_true_in_goal = false,
             bool is_derived = false)
        : atoms(move(_atoms)), is_disjunctive(is_disjunctive),
          is_conjunctive(is_conjunctive), is_true_in_goal(is_true_in_goal),
          is_derived(is_derived) {
        assert(!(is_conjunctive && is_disjunctive));
        assert((is_conjunctive && atoms.size() > 1) ||
               (is_disjunctive && atoms.size() > 1) || atoms.size() == 1);
    }

    bool operator==(const Landmark &other) const {
        return this == &other;
    }

    bool operator!=(const Landmark &other) const {
        return !(*this == other);
    }

    std::vector<FactPair> atoms;
    const bool is_disjunctive;
    const bool is_conjunctive;
    bool is_true_in_goal;
    bool is_derived;

    std::unordered_set<int> first_achievers;
    std::unordered_set<int> possible_achievers;

    bool is_true_in_state(const State &state) const;
    bool contains(const FactPair &atom) const;
};
}
#endif

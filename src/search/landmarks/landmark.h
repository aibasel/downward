#ifndef LANDMARKS_LANDMARK_H
#define LANDMARKS_LANDMARK_H

#include "../task_proxy.h"

#include <unordered_set>

namespace landmarks {
enum LandmarkType {
    DISJUNCTIVE,
    SIMPLE,
    CONJUNCTIVE,
};
class Landmark {
public:
    Landmark(std::vector<FactPair> _atoms, LandmarkType type,
             bool is_true_in_goal = false, bool is_derived = false)
        : atoms(move(_atoms)), type(type),
          is_true_in_goal(is_true_in_goal), is_derived(is_derived) {
        assert((type == DISJUNCTIVE && atoms.size() > 1) ||
               (type == CONJUNCTIVE && atoms.size() > 1) ||
               (type == SIMPLE && atoms.size() == 1));
    }

    bool operator==(const Landmark &other) const {
        return this == &other;
    }

    bool operator!=(const Landmark &other) const {
        return !(*this == other);
    }

    const std::vector<FactPair> atoms;
    const LandmarkType type;
    bool is_conjunctive;
    bool is_true_in_goal;
    bool is_derived;

    std::unordered_set<int> first_achievers;
    std::unordered_set<int> possible_achievers;

    bool is_true_in_state(const State &state) const;
    bool contains(const FactPair &atom) const;
};
}
#endif

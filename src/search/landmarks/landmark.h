#ifndef LANDMARKS_LANDMARK_H
#define LANDMARKS_LANDMARK_H

#include "../task_proxy.h"

#include <unordered_set>

namespace landmarks {
/*
  Here, landmarks are formulas over the atoms of the planning task. We support
  exactly three specific kinds of formulas: atomic formulas, disjunctions, and
  conjunctions. Therefore, we can represent the landmarks as sets of atoms
  annotated with their type `ATOMIC`, `DISJUNCTIVE`, or `CONJUNCTIVE`. We assert
  that `ATOMIC` landmarks consist of exactly one atom. Even though atomic
  formulas can in theory be considered to be disjunctions or conjunctions over
  a single atom, we require that `DISJUNCTIVE` and `CONJUNCTIVE` landmarks
  consist of at least two atoms.
*/
enum LandmarkType {
    DISJUNCTIVE,
    ATOMIC,
    CONJUNCTIVE,
};

class Landmark {
public:
    Landmark(std::vector<FactPair> _atoms, LandmarkType type,
             bool is_true_in_goal = false, bool is_derived = false)
        : atoms(move(_atoms)), type(type),
          is_true_in_goal(is_true_in_goal), is_derived(is_derived) {
        assert((type == ATOMIC && atoms.size() == 1) ||
               (type != ATOMIC && atoms.size() > 1));
    }

    bool operator==(const Landmark &other) const {
        return this == &other;
    }

    bool operator!=(const Landmark &other) const {
        return !(*this == other);
    }

    const std::vector<FactPair> atoms;
    const LandmarkType type;
    bool is_true_in_goal;
    bool is_derived;

    std::unordered_set<int> first_achievers;
    std::unordered_set<int> possible_achievers;

    bool is_true_in_state(const State &state) const;
    bool contains(const FactPair &atom) const;
};
}
#endif

#ifndef LANDMARKS_LANDMARK_H
#define LANDMARKS_LANDMARK_H

#include "../task_proxy.h"

#include <set>

namespace landmarks {
enum class LandmarkType {
    SIMPLE,
    DISJUNCTIVE,
    CONJUNCTIVE
};

class Landmark {
public:
    Landmark(std::vector<FactPair> facts,
                      bool is_true_in_goal = false, bool is_derived = false)
        : facts(move(facts)),
          is_true_in_goal(is_true_in_goal), is_derived(is_derived) {
        assert(!this->facts.empty());
    }

    virtual ~Landmark() = default;

    bool operator ==(const Landmark &other) const {
        return this == &other;
    }

    bool operator !=(const Landmark &other) const {
        return !(*this == other);
    }

    std::vector<FactPair> facts;
    bool is_true_in_goal;
    bool is_derived;

    std::set<int> first_achievers;
    std::set<int> possible_achievers;

    virtual bool is_true_in_state(const State &state) const = 0;
    virtual LandmarkType get_type() const = 0;
};
}
#endif

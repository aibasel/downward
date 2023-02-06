#ifndef LANDMARKS_CONJUNCTIVE_LANDMARK_H
#define LANDMARKS_CONJUNCTIVE_LANDMARK_H

#include "../task_proxy.h"
#include "landmark.h"
#include <set>

namespace landmarks {
class ConjunctiveLandmark : public Landmark {
public:
    ConjunctiveLandmark(std::vector<FactPair> _facts,
             bool is_true_in_goal = false, bool is_derived = false)
        : Landmark(_facts, is_true_in_goal, is_derived) {
        assert(facts.size() > 1);
    }


    bool is_true_in_state(const State &state) const override;
    LandmarkType get_type() const override;
};
}
#endif

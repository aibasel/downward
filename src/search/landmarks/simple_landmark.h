#ifndef LANDMARKS_SIMPLE_LANDMARK_H
#define LANDMARKS_SIMPLE_LANDMARK_H

#include "landmark.h"

namespace landmarks {
class SimpleLandmark : public Landmark {
public:
    SimpleLandmark(std::vector<FactPair> facts,
                   bool is_true_in_goal = false, bool is_derived = false)
        : Landmark(facts, is_true_in_goal, is_derived) {
        assert(this->facts.size() == 1);
    }

    bool is_true_in_state(const State &state) const override;
    LandmarkType get_type() const override;
};
}
#endif

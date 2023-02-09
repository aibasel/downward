#include "landmark.h"
#include "simple_landmark.h"

using namespace std;

namespace landmarks {
bool SimpleLandmark::is_true_in_state(const State &state) const {
    return state[facts.front().var].get_value() == facts.front().value;
}


LandmarkType SimpleLandmark::get_type() const {
    return LandmarkType::SIMPLE;
}
}

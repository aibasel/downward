#include "landmark.h"
#include "conjunctive_landmark.h"

using namespace std;

namespace landmarks {
bool ConjunctiveLandmark::is_true_in_state(const State &state) const {
    for (const FactPair &fact : facts) {
        if (state[fact.var].get_value() != fact.value) {
            return false;
        }
    }
    return true;
}


LandmarkType ConjunctiveLandmark::get_type() const {
    return LandmarkType::CONJUNCTIVE;
}
}

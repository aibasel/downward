#include "disjunctive_landmark.h"


using namespace std;

namespace landmarks {
bool DisjunctiveLandmark::is_true_in_state(const State &state) const {
    for (const FactPair &fact : facts) {
        if (state[fact.var].get_value() == fact.value) {
            return true;
        }
    }
    return false;
}

LandmarkType DisjunctiveLandmark::get_type() const {
    return LandmarkType::DISJUNCTIVE;
}
}

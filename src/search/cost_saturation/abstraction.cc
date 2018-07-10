#include "abstraction.h"

#include "../utils/collections.h"

#include <cassert>

using namespace std;

namespace cost_saturation {
Abstraction::Abstraction()
    : has_transition_system_(true) {
}

bool Abstraction::has_transition_system() const {
    return has_transition_system_;
}

pair<vector<int>, vector<int>> Abstraction::compute_goal_distances_and_saturated_costs(
    const vector<int> &costs) const {
    assert(has_transition_system());
    vector<int> h_values = compute_goal_distances(costs);
    int num_operators = costs.size();
    vector<int> saturated_costs = compute_saturated_costs(
        h_values, num_operators);
    return make_pair(move(h_values), move(saturated_costs));
}

void Abstraction::remove_transition_system() {
    assert(has_transition_system());
    release_transition_system_memory();
    has_transition_system_ = false;
}
}

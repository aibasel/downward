#include "abstraction.h"

#include "../utils/collections.h"

#include <cassert>

using namespace std;

namespace cost_saturation {
Abstraction::Abstraction()
    : has_transition_system(true) {
}

Abstraction::~Abstraction() {
}

pair<vector<int>, vector<int>> Abstraction::compute_goal_distances_and_saturated_costs(
    const vector<int> &costs,
    bool use_general_costs) const {
    vector<int> h_values = compute_h_values(costs);
    int num_operators = costs.size();
    vector<int> saturated_costs = compute_saturated_costs(
        h_values, num_operators, use_general_costs);
    return make_pair(move(h_values), move(saturated_costs));
}

const std::vector<int> &Abstraction::get_active_operators() const {
    assert(has_transition_system);
    return active_operators;
}

const std::vector<int> &Abstraction::get_looping_operators() const {
    assert(has_transition_system);
    return looping_operators;
}

const vector<int> &Abstraction::get_goal_states() const {
    assert(has_transition_system);
    return goal_states;
}

void Abstraction::remove_transition_system() {
    assert(has_transition_system);
    utils::release_vector_memory(active_operators);
    utils::release_vector_memory(looping_operators);
    utils::release_vector_memory(goal_states);
    has_transition_system = false;
}
}

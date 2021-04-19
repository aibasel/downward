#include "pattern_database.h"

#include "../utils/math.h"

using namespace std;

namespace pdbs {
PatternDatabase::PatternDatabase(
    const Pattern &pattern,
    std::size_t num_states,
    std::vector<int> &&distances,
    std::vector<std::size_t> &&hash_multipliers)
    : pattern(pattern),
      num_states(num_states),
      distances(move(distances)),
      hash_multipliers(move(hash_multipliers)) {
}

size_t PatternDatabase::hash_index_of_concrete_state(const vector<int> &state) const {
    size_t index = 0;
    for (size_t i = 0; i < pattern.size(); ++i) {
        index += hash_multipliers[i] * state[pattern[i]];
    }
    return index;
}

size_t PatternDatabase::hash_index_of_projected_state(const State &projected_state) const {
    size_t index = 0;
    for (size_t i = 0; i < pattern.size(); ++i) {
        index += hash_multipliers[i] * projected_state[i].get_value();
    }
    return index;
}

int PatternDatabase::get_value(const vector<int> &state) const {
    return distances[hash_index_of_concrete_state(state)];
}

int PatternDatabase::get_value_for_hash_index(size_t index) const {
    return distances[index];
}

double PatternDatabase::compute_mean_finite_h() const {
    double sum = 0;
    int size = 0;
    for (size_t i = 0; i < distances.size(); ++i) {
        if (distances[i] != numeric_limits<int>::max()) {
            sum += distances[i];
            ++size;
        }
    }
    if (size == 0) { // All states are dead ends.
        return numeric_limits<double>::infinity();
    } else {
        return sum / size;
    }
}

bool PatternDatabase::is_operator_relevant(const OperatorProxy &op) const {
    for (EffectProxy effect : op.get_effects()) {
        int var_id = effect.get_fact().get_variable().get_id();
        if (binary_search(pattern.begin(), pattern.end(), var_id)) {
            return true;
        }
    }
    return false;
}
}

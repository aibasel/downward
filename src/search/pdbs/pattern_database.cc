#include "pattern_database.h"

#include <limits>
#include <vector>

using namespace std;

namespace pdbs {
PatternDatabase::PatternDatabase(
    Pattern &&pattern,
    int num_states,
    vector<int> &&distances,
    vector<int> &&hash_multipliers)
    : pattern(move(pattern)),
      num_states(num_states),
      distances(move(distances)),
      hash_multipliers(move(hash_multipliers)) {
}

int PatternDatabase::hash_index(const vector<int> &state) const {
    int index = 0;
    for (size_t i = 0; i < pattern.size(); ++i) {
        index += hash_multipliers[i] * state[pattern[i]];
    }
    return index;
}

int PatternDatabase::get_value(const vector<int> &state) const {
    return distances[hash_index(state)];
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

#include "pattern_database.h"

#include "../task_proxy.h"

using namespace std;

namespace pdbs {
PerfectHashFunction::PerfectHashFunction(
    Pattern &&pattern,
    size_t num_states,
    vector<size_t> &&hash_multipliers)
    : pattern(move(pattern)),
      num_states(num_states),
      hash_multipliers(move(hash_multipliers)) {
}

size_t PerfectHashFunction::rank(const vector<int> &state) const {
    size_t index = 0;
    for (size_t i = 0; i < pattern.size(); ++i) {
        index += hash_multipliers[i] * state[pattern[i]];
    }
    return index;
}

int PerfectHashFunction::unrank(size_t index, int var, int domain_size) const {
    int temp = index / hash_multipliers[var];
    return temp % domain_size;
}

PatternDatabase::PatternDatabase(
    PerfectHashFunction &&hash_function,
    vector<int> &&distances)
    : hash_function(move(hash_function)),
      distances(move(distances)) {
}

int PatternDatabase::get_value(const vector<int> &state) const {
    return distances[hash_function.rank(state)];
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
        if (binary_search(hash_function.get_pattern().begin(),
                          hash_function.get_pattern().end(),
                          var_id)) {
            return true;
        }
    }
    return false;
}
}

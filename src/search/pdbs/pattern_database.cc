#include "pattern_database.h"

#include <limits>
#include <vector>

using namespace std;

namespace pdbs {
Projection::Projection(
    Pattern &&pattern,
    int num_abstract_states,
    vector<int> &&hash_multipliers)
    : pattern(move(pattern)),
      num_abstract_states(num_abstract_states),
      hash_multipliers(move(hash_multipliers)) {
}

int Projection::rank(const vector<int> &state) const {
    size_t index = 0;
    for (size_t i = 0; i < pattern.size(); ++i) {
        index += hash_multipliers[i] * state[pattern[i]];
    }
    return index;
}

int Projection::unrank(int index, int var, int domain_size) const {
    int temp = index / hash_multipliers[var];
    return temp % domain_size;
}

PatternDatabase::PatternDatabase(
    Projection &&projection,
    vector<int> &&distances)
    : projection(move(projection)),
      distances(move(distances)) {
}

int PatternDatabase::get_value(const vector<int> &state) const {
    return distances[projection.rank(state)];
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
}

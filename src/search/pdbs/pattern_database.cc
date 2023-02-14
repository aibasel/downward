#include "pattern_database.h"

#include "../task_utils/task_properties.h"

#include "../utils/logging.h"
#include "../utils/math.h"

#include <cassert>
#include <iostream>
#include <limits>
#include <vector>

using namespace std;

namespace pdbs {
Projection::Projection(
    const TaskProxy &task_proxy, const Pattern &pattern)
    : pattern(pattern) {
    task_properties::verify_no_axioms(task_proxy);
    task_properties::verify_no_conditional_effects(task_proxy);
    assert(utils::is_sorted_unique(pattern));

    domain_sizes.reserve(pattern.size());
    hash_multipliers.reserve(pattern.size());
    num_abstract_states = 1;
    for (int pattern_var_id : pattern) {
        hash_multipliers.push_back(num_abstract_states);
        VariableProxy var = task_proxy.get_variables()[pattern_var_id];
        int domain_size = var.get_domain_size();
        domain_sizes.push_back(domain_size);
        if (utils::is_product_within_limit(
                num_abstract_states,
                domain_size,
                numeric_limits<int>::max())) {
            num_abstract_states *= domain_size;
        } else {
            cerr << "Given pattern is too large! (Overflow occured): " << endl;
            cerr << pattern << endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
    }
}

int Projection::rank(const vector<int> &state) const {
    size_t index = 0;
    for (size_t i = 0; i < pattern.size(); ++i) {
        index += hash_multipliers[i] * state[pattern[i]];
    }
    return index;
}

int Projection::unrank(int index, int var) const {
    int temp = index / hash_multipliers[var];
    return temp % domain_sizes[var];
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

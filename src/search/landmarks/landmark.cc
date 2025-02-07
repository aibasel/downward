#include "landmark.h"

using namespace std;

namespace landmarks {
bool Landmark::is_true_in_state(const State &state) const {
    auto is_fact_true_in_state = [&](const FactPair &fact) {
            return state[fact.var].get_value() == fact.value;
        };
    if (is_disjunctive) {
        return any_of(facts.cbegin(), facts.cend(), is_fact_true_in_state);
    } else {
        // Is conjunctive or simple.
        return all_of(facts.cbegin(), facts.cend(), is_fact_true_in_state);
    }
}
}

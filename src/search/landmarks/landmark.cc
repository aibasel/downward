#include "landmark.h"

using namespace std;

namespace landmarks {
bool Landmark::is_true_in_state(const State &state) const {
    auto is_atom_true_in_state = [&](const FactPair &atom) {
            return state[atom.var].get_value() == atom.value;
        };
    if (is_disjunctive) {
        return ranges::any_of(
            atoms.cbegin(), atoms.cend(), is_atom_true_in_state);
    } else {
        // Is conjunctive or simple.
        return ranges::all_of(
            atoms.cbegin(), atoms.cend(), is_atom_true_in_state);
    }
}
}

#include "landmark.h"

using namespace std;

namespace landmarks {
bool Landmark::is_true_in_state(const State &state) const {
    auto is_atom_true_in_state = [&](const FactPair &atom) {
            return state[atom.var] == atom.value;
        };
    if (type == DISJUNCTIVE) {
        return ranges::any_of(
            atoms.cbegin(), atoms.cend(), is_atom_true_in_state);
    } else {
        assert(type == CONJUNCTIVE || type == ATOMIC);
        return ranges::all_of(
            atoms.cbegin(), atoms.cend(), is_atom_true_in_state);
    }
}

bool Landmark::contains(const FactPair &atom) const {
    return find(atoms.begin(), atoms.end(), atom) != atoms.end();
}
}

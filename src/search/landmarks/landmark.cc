#include "landmark.h"

#include <ranges>

using namespace std;

namespace landmarks {
bool Landmark::is_true_in_state(const State &state) const {
    auto is_atom_true_in_state = [&](const FactPair &atom) {
            return state[atom.var].get_value() == atom.value;
        };
    if (type == DISJUNCTIVE) {
        return ranges::any_of(atoms, is_atom_true_in_state);
    } else {
        assert(type == CONJUNCTIVE || type == ATOMIC);
        return ranges::all_of(atoms, is_atom_true_in_state);
    }
}

bool Landmark::contains(const FactPair &atom) const {
    return ranges::find(atoms, atom) != atoms.end();
}
}

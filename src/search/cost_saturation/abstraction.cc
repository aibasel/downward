#include "abstraction.h"

#include <cassert>

using namespace std;

namespace cost_saturation {
Abstraction::Abstraction(unique_ptr<AbstractionFunction> abstraction_function)
    : abstraction_function(move(abstraction_function)) {
}

int Abstraction::get_abstract_state_id(const State &concrete_state) const {
    assert(abstraction_function);
    return abstraction_function->get_abstract_state_id(concrete_state);
}

unique_ptr<AbstractionFunction> Abstraction::extract_abstraction_function() {
    return move(abstraction_function);
}
}

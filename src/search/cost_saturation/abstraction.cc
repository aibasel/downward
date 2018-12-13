#include "abstraction.h"

#include "../utils/collections.h"

#include <cassert>

using namespace std;

namespace cost_saturation {
Abstraction::Abstraction()
    : has_transition_system_(true) {
}

bool Abstraction::has_transition_system() const {
    return has_transition_system_;
}

void Abstraction::remove_transition_system() {
    assert(has_transition_system());
    release_transition_system_memory();
    has_transition_system_ = false;
}
}

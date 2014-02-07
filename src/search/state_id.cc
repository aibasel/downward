#include "state_id.h"

#include <ostream>

using namespace std;

const StateID StateID::no_state = StateID(-1);

ostream &operator<<(ostream &os, StateID id) {
    os << "#"  << id.value;
    return os;
}

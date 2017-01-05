#include "action_id.h"

#include <ostream>

using namespace std;

ostream &operator<<(ostream &os, ActionID id) {
    if (id.value >= 0) {
        os << "op" << id.value;
    } else {
        os << "axiom" << -1 - id.value;
    }
    return os;
}

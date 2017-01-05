#include "action_id.h"

#include <ostream>

using namespace std;

ostream &operator<<(ostream &os, ActionID id) {
    if (id.is_axiom()) {
        os << "axiom" << id.get_index();
    } else {
        os << "op" << id.get_index();
    }
    return os;
}

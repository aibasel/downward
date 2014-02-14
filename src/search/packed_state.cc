#include "packed_state.h"

using namespace std;

PackedStateProperties::PackedStateProperties(const vector<int> &variable_domain) {
    // TODO use less than one PackedStateEntry per variable
    state_size = variable_domain.size();
}

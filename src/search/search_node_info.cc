#include "search_node_info.h"

#include "utils/system.h"

using namespace std;

static_assert(
    sizeof(SearchNodeInfo) == sizeof(StateID) + sizeof(int),
    "SearchNodeInfo has unexpected size. This probably means that packing two "
    "fields into one integer using bitfields is not supported.");

// Maximum positive 30-bit integer.
static const int MAX_OP_ID = (1 << 29) - 1;

OperatorID SearchNodeInfo::get_creating_operator_id() const {
    return OperatorID(creating_operator);
}

void SearchNodeInfo::set_creating_operator_id(OperatorID op_id) {
    if (op_id.get_index() > MAX_OP_ID) {
        ABORT("Operator ID is too large.");
    }
    creating_operator = op_id.get_index();
}

#ifndef SEARCH_NODE_INFO_H
#define SEARCH_NODE_INFO_H

#include "operator_id.h"
#include "state_id.h"

// For documentation on classes relevant to storing and working with registered
// states see the file state_registry.h.

struct SearchNodeInfo {
    enum NodeStatus {NEW = 0, OPEN = 1, CLOSED = 2, DEAD_END = 3};

    unsigned int status : 2;
    int creating_operator : OperatorID::num_bits;
    StateID parent_state_id;

    SearchNodeInfo()
        : status(NEW), creating_operator(OperatorID::no_operator.get_index()),
          parent_state_id(StateID::no_state) {
    }

    OperatorID get_creating_operator_id() const {
        return OperatorID(creating_operator);
    }
    void set_creating_operator_id(OperatorID op_id) {
        creating_operator = op_id.get_index();
    }
};

#endif

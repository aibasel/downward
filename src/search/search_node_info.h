#ifndef SEARCH_NODE_INFO_H
#define SEARCH_NODE_INFO_H

#include "operator_id.h"
#include "state_id.h"

// For documentation on classes relevant to storing and working with registered
// states see the file state_registry.h.

struct SearchNodeInfo {
    enum NodeStatus {NEW = 0, OPEN = 1, CLOSED = 2, DEAD_END = 3};

    // TODO: pack this into 8 bytes.
    NodeStatus status;
    StateID parent_state_id;
    OperatorID creating_operator;

    SearchNodeInfo()
        : status(NEW), parent_state_id(StateID::no_state),
          creating_operator(OperatorID::no_operator) {
    }
};

#endif

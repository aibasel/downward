#ifndef SEARCH_NODE_INFO_H
#define SEARCH_NODE_INFO_H

#include "state.h"

class SearchNodeInfo {
    friend class SearchNode;
    friend class SearchSpace;

    enum NodeStatus {NEW = 0, OPEN = 1, CLOSED = 2, DEAD_END = 3};

    unsigned int status : 2;
    int g : 30;
    int h : 31; // TODO:CR - should we get rid of it
    bool h_is_dirty : 1;
    const state_var_t *parent_state;
    const Operator *creating_operator;
    int real_g;

    SearchNodeInfo()
        : status(NEW), g(-1), h(-1), h_is_dirty(false),
          parent_state(0), creating_operator(0), real_g(-1) {
    }
};

/*
  TODO: The C++ standard does not guarantee that bitfields with mixed
        types (unsigned int, int, bool) are stored in the compact way
        we desire. However, g++ does do what we want. To be safe for
        the future, we should add a static assertion that verifies
        that SearchNodeInfo has the desired size.
 */

#endif

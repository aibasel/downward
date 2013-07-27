#ifndef SEARCH_NODE_INFO_H
#define SEARCH_NODE_INFO_H

#include "state.h"
#include "per_state_information.h"
#include "segmented_vector.h" // HACK

class SearchNodeInfo {
    // HACK: Get rid of some of these friends
    friend class SearchNode;
    friend class SearchSpace;
    friend class PerStateInformation<SearchNodeInfo>;
    friend class SegmentedVector<SearchNodeInfo>;

    enum NodeStatus {NEW = 0, OPEN = 1, CLOSED = 2, DEAD_END = 3};

    unsigned int status : 2;
    int g : 30;
    int h : 31; // TODO:CR - should we get rid of it
    bool h_is_dirty : 1;
    StateHandle parent_state_handle;
    const Operator *creating_operator;
    int real_g;

    SearchNodeInfo()
        : status(NEW), g(-1), h(-1), h_is_dirty(false),
          parent_state_handle(StateHandle::invalid), creating_operator(0),
          real_g(-1) {
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

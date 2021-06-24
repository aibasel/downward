#include "search_node_info.h"

static_assert(
    sizeof(SearchNodeInfo) == sizeof(StateID) + sizeof(int),
    "SearchNodeInfo has unexpected size.");

#include "search_node_info.h"

static_assert(
    sizeof(SearchNodeInfo) == sizeof(StateID) + sizeof(int),
    "SearchNodeInfo has unexpected size. This probably means that packing two "
    "fields into one integer using bitfields is not supported.");

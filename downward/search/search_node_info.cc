#include "search_node_info.h"

static const int pointer_bytes = sizeof(void *);
static const int info_bytes = 3 * sizeof(int) + sizeof(StateID);
static const int padding_bytes = info_bytes % pointer_bytes;

static_assert(
    sizeof(SearchNodeInfo) == info_bytes + padding_bytes,
    "The size of SearchNodeInfo is larger than expected. This probably means "
    "that packing two fields into one integer using bitfields is not supported.");

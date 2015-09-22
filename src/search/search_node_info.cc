#include "search_node_info.h"

/*
  The C++ standard does not guarantee that bitfields with mixed types
  (unsigned int, int, bool) are stored in the compact way we desire. We
  use the static assertion below to verify that SearchNodeInfo has the
  desired size.
*/

static_assert(
    sizeof(SearchNodeInfo) ==
    3 * sizeof(int) + sizeof(StateID) + sizeof(GlobalOperator *),
    "SearchNodeInfo is too big");

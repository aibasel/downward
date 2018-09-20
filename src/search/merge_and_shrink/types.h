#ifndef MERGE_AND_SHRINK_TYPES_H
#define MERGE_AND_SHRINK_TYPES_H

#include <forward_list>
#include <list>
#include <vector>

namespace merge_and_shrink {
// Positive infinity. The name "INFINITY" is taken by an ISO C99 macro.
extern const int INF;
extern const int MINUSINF;
extern const int PRUNED_STATE;

/*
  An equivalence class is a set of abstract states that shall be
  mapped (shrunk) to the same abstract state.

  An equivalence relation is a partitioning of states into
  equivalence classes. It may omit certain states entirely; these
  will be dropped completely and receive an h value of infinity.
*/
using StateEquivalenceClass = std::forward_list<int>;
using StateEquivalenceRelation = std::vector<StateEquivalenceClass>;

enum class Verbosity {
    SILENT,
    NORMAL,
    VERBOSE
};
}

#endif

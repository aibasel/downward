#ifndef MERGE_AND_SHRINK_TYPES_H
#define MERGE_AND_SHRINK_TYPES_H

#include <forward_list>
#include <list>
#include <vector>

// Related to representation of grouped labels
typedef std::list<int>::const_iterator LabelConstIter;

// Positive infinity. The name "INFINITY" is taken by an ISO C99 macro.
extern const int INF;

/* An equivalence class is a set of abstract states that shall be
   mapped (shrunk) to the same abstract state.

   An equivalence relation is a partitioning of states into
   equivalence classes. It may omit certain states entirely; these
   will be dropped completely and receive an h value of infinity.
*/
typedef std::forward_list<int> StateEquivalenceClass;
typedef std::vector<StateEquivalenceClass> StateEquivalenceRelation;

#endif

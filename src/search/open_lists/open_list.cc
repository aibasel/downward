// HACK! Ignore this if used as a top-level compile target.
#ifdef OPEN_LISTS_OPEN_LIST_H

#include "../evaluation_context.h"


template<class Entry>
OpenList<Entry>::OpenList(bool only_preferred)
    : only_preferred(only_preferred) {
}

template<class Entry>
bool OpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &/*eval_context*/, const Entry &/*entry*/) {
    return false;
}

template<class Entry>
void OpenList<Entry>::boost_preferred() {
}

template<class Entry>
void OpenList<Entry>::insert(
    EvaluationContext &eval_context, const Entry &entry) {
    if (only_preferred && !eval_context.is_preferred())
        return;
    if (is_reliable_dead_end(eval_context, entry))
        return;
    do_insertion(eval_context, entry);
}

#endif

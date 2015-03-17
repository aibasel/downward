// HACK! Ignore this if used as a top-level compile target.
#ifdef OPEN_LISTS_OPEN_LIST_H

#include "../evaluation_context.h"


template<class Entry>
OpenList<Entry>::OpenList(bool only_preferred)
    : only_preferred(only_preferred) {
}

template<class Entry>
void OpenList<Entry>::boost_preferred() {
}

template<class Entry>
void OpenList<Entry>::insert(
    EvaluationContext &eval_context, const Entry &entry) {
    if (only_preferred && !eval_context.is_preferred())
        return;
    if (!is_dead_end(eval_context))
        do_insertion(eval_context, entry);
}

template<class Entry>
bool OpenList<Entry>::only_contains_preferred_entries() const {
    return only_preferred;
}

#endif

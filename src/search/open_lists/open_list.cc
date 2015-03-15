// HACK! Ignore this if used as a top-level compile target.
#ifdef OPEN_LISTS_OPEN_LIST_H

template<class Entry>
OpenList<Entry>::OpenList(bool only_preferred)
    : only_preferred(only_preferred) {
}

template<class Entry>
virtual bool is_reliable_dead_end(
    EvaluationContext &/*eval_context*/, const Entry &/*entry*/) {
    return false;
}

template<class Entry>
void OpenList<Entry>::boost_preferred() {
}

#endif

#ifndef CLOSED_LIST_H
#define CLOSED_LIST_H

#include <map>
#include <vector>

template<class Entry, class Annotation>
class ClosedList {
    struct PredecessorInfo {
        const Entry *predecessor;
        Annotation annotation;
        PredecessorInfo(const Entry *pred, const Annotation &annote)
            : predecessor(pred), annotation(annote) {}
    };

    typedef std::map<Entry, PredecessorInfo> ClosedListMap;
    ClosedListMap closed;
public:
    ClosedList();
    ~ClosedList();
    const Entry *insert(const Entry &entry,
                        const Entry *predecessor,
                        const Annotation &annotation);
    void clear();

    bool contains(const Entry &entry) const;
    int size() const;
    void trace_path(const Entry &entry, std::vector<Annotation> &path) const;
};

#include "closed_list.cc" // HACK! Templates and the current Makefile don't mix well

#endif

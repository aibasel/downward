#ifndef OPEN_LIST_BUCKETS_H
#define OPEN_LIST_BUCKETS_H

#include <deque>
#include <utility>
#include <vector>

template<class Entry>
class OpenList {
    typedef std::deque<Entry> Bucket;
    std::vector<Bucket> buckets;
    mutable int lowest_bucket;
    int size;
public:
    OpenList();
    ~OpenList();

    void insert(int key, const Entry &entry);
    Entry remove_min();
    void clear();

    int min() const;
    bool empty() const;
};

#include "open_list_buckets.cc"

// HACK! Need a better strategy of dealing with templates, also in the Makefile.

#endif

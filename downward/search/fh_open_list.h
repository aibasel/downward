#ifndef FH_OPEN_LIST_H
#define FH_OPEN_LIST_H

#include <deque>
#include <utility>
#include <vector>
#include <iostream>

// class State;
// class Operator;

template<class Entry>
class FhOpenList {
    // typedef std::pair<const State *, const Operator *> Entry;
    typedef std::deque<Entry> Bucket;

    std::vector<std::vector<Bucket> > fbuckets;

    std::vector<int> lowest_hbucket;
    std::vector<int> fsize;

    mutable int lowest_fbucket;
    int size;
public:
    FhOpenList();
    ~FhOpenList();

    // void insert(int key, const State *parent, const Operator *operator_);
    void insert(int fkey, int hkey, const Entry &entry);
    Entry remove_min();
    void clear();

    bool empty() const;

    void print();
};

#include "fh_open_list.cc"

// HACK! Need a better strategy of dealing with templates, also in the Makefile.

#endif

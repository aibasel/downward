// HACK! Ignore this if used as a top-level compile target.
#ifdef OPEN_LIST_BUCKETS_H

#include <cassert>
using namespace std;

/*
  Bucket-based implementation of an open list.
  Nodes with identical heuristic value are expanded in FIFO order.

  It would be easy to templatize the "State *" and "Operator *"
  datatypes, because these are only used as anonymous data. However,
  there is little point in applying such generalizations before there
  is any need for them.
*/

template<class Entry>
OpenList<Entry>::OpenList() {
    lowest_bucket = 0;
    size = 0;
}

template<class Entry>
OpenList<Entry>::~OpenList() {
}

template<class Entry>
void OpenList<Entry>::insert(int key, const Entry &entry) {
    assert(key >= 0);
    if(key >= buckets.size())
	buckets.resize(key + 1);
    else if(key < lowest_bucket)
	lowest_bucket = key;
    buckets[key].push_back(entry);
    size++;
}
/*
void OpenList::insert(int key, const State *parent, const Operator *operator_) {
    assert(key >= 0);
    if(key >= buckets.size())
	buckets.resize(key + 1);
    else if(key < lowest_bucket)
	lowest_bucket = key;
    buckets[key].push_back(Entry(parent, operator_));
    size++;
}
*/

template<class Entry>
int OpenList<Entry>::min() const {
    assert(size > 0);
    while(buckets[lowest_bucket].empty())
	lowest_bucket++;
    return lowest_bucket;
}

template<class Entry>
Entry OpenList<Entry>::remove_min() {
    assert(size > 0);
    while(buckets[lowest_bucket].empty())
	lowest_bucket++;
    size--;
    Entry result = buckets[lowest_bucket].front();
    buckets[lowest_bucket].pop_front();
    return result;
}

template<class Entry>
bool OpenList<Entry>::empty() const {
    return size == 0;
}

template<class Entry>
void OpenList<Entry>::clear() {
    buckets.clear();
    lowest_bucket = 0;
    size = 0;
}

#endif

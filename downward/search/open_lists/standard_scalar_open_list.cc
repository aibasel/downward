// HACK! Ignore this if used as a top-level compile target.
#ifdef OPEN_LISTS_STANDARD_SCALAR_OPEN_LIST_H

#include "../scalar_evaluator.h"

#include <iostream>
#include <cassert>
using namespace std;

/*
  Bucket-based implementation of a open list.
  Nodes with identical heuristic value are expanded in FIFO order.
*/

template<class Entry>
StandardScalarOpenList<Entry>::StandardScalarOpenList(ScalarEvaluator *eval,
    bool preferred_only)
    : OpenList<Entry>(preferred_only), size(0), evaluator(eval) {
    lowest_bucket = 9999999;
}

template<class Entry>
StandardScalarOpenList<Entry>::~StandardScalarOpenList() {
}

template<class Entry>
int StandardScalarOpenList<Entry>::insert(const Entry &entry) {
    if (OpenList<Entry>::only_preferred && !last_preferred)
        return 0;
    int key = last_evaluated_value;
    if(key < lowest_bucket)
        lowest_bucket = key;
    buckets[key].push_back(entry);
    size++;
    return 1;
}

template<class Entry>
Entry StandardScalarOpenList<Entry>::remove_min() {
    assert(size > 0);
    typename std::map<int, Bucket>::iterator it;
    it = buckets.find(lowest_bucket); 
    assert(it != buckets.end());
    while(it->second.empty())
        ++ it;
    assert(it != buckets.end());
    lowest_bucket = it->first;
    size--;
    Entry result = it->second.front();
    it->second.pop_front();
    return result;
}

template<class Entry>
bool StandardScalarOpenList<Entry>::empty() const {
    return size == 0;
}

template<class Entry>
void StandardScalarOpenList<Entry>::clear() {
    buckets.clear();
    lowest_bucket = 9999999;
    size = 0;
}

template<class Entry>
void StandardScalarOpenList<Entry>::evaluate(int g, bool preferred) {
    get_evaluator()->evaluate(g, preferred);
    last_evaluated_value = get_evaluator()->get_value();
    last_preferred = preferred;
    dead_end = get_evaluator()->is_dead_end();
    dead_end_reliable = get_evaluator()->dead_end_is_reliable();
}

template<class Entry>
bool StandardScalarOpenList<Entry>::is_dead_end() const {
    return dead_end;
}

template<class Entry>
bool StandardScalarOpenList<Entry>::dead_end_is_reliable() const {
    return dead_end_reliable;
}

#endif

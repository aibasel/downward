// HACK! Ignore this if used as a top-level compile target.
#ifdef OPEN_LISTS_OPEN_LIST_BUCKETS_H

#include <cassert>
#include <limits>
#include "../option_parser.h"

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
OpenList<Entry> *BucketOpenList<Entry>::_parse(OptionParser &parser) {
    parser.add_list_option<ScalarEvaluator *>("evals");
    parser.add_option<bool>("pref_only", false,
                            "insert only preferred operators");
    Options opts = parser.parse();

    opts.verify_list_non_empty<ScalarEvaluator *>("evals");

    if (parser.dry_run())
        return 0;
    else
        return new BucketOpenList<Entry>(opts);
}


template<class Entry>
BucketOpenList<Entry>::BucketOpenList(const Options &opts)
    : OpenList<Entry>(opts.get<bool>("pref_only")),
      lowest_bucket(numeric_limits<int>::max()),
      size(0), evaluator(opts.get_list<ScalarEvaluator *>("evals")[0]) {
}

template<class Entry>
BucketOpenList<Entry>::~BucketOpenList() {
}

template<class Entry>
int BucketOpenList<Entry>::insert(const Entry &entry) {
    if (OpenList<Entry>::only_preferred && !last_preferred)
        return 0;
    if (dead_end) {
        return 0;
    }
    int key = last_evaluated_value;
    assert(key >= 0);
    if (key >= buckets.size())
        buckets.resize(key + 1);
    if (key < lowest_bucket)
        lowest_bucket = key;
    buckets[key].push_back(entry);
    size++;
    return 1;
}

template<class Entry>
Entry BucketOpenList<Entry>::remove_min(vector<int> *key) {
    assert(size > 0);
    while (buckets[lowest_bucket].empty())
        lowest_bucket++;
    size--;
    if (key) {
        assert(key->empty());
        key->push_back(lowest_bucket);
    }
    Entry result = buckets[lowest_bucket].front();
    buckets[lowest_bucket].pop_front();
    return result;
}


template<class Entry>
bool BucketOpenList<Entry>::empty() const {
    return size == 0;
}

template<class Entry>
void BucketOpenList<Entry>::clear() {
    buckets.clear();
    lowest_bucket = numeric_limits<int>::max();
    size = 0;
}

template<class Entry>
void BucketOpenList<Entry>::evaluate(int g, bool preferred) {
    get_evaluator()->evaluate(g, preferred);
    last_evaluated_value = get_evaluator()->get_value();
    last_preferred = preferred;
    dead_end = get_evaluator()->is_dead_end();
    dead_end_reliable = get_evaluator()->dead_end_is_reliable();
}

template<class Entry>
bool BucketOpenList<Entry>::is_dead_end() const {
    return dead_end;
}

template<class Entry>
bool BucketOpenList<Entry>::dead_end_is_reliable() const {
    return dead_end_reliable;
}

template<class Entry>
void BucketOpenList<Entry>::get_involved_heuristics(std::set<Heuristic *> &hset) {
    evaluator->get_involved_heuristics(hset);
}
#endif

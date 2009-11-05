// HACK! Ignore this if used as a top-level compile target.
#ifdef TIEBREAKING_OPENLIST_H

#include <iostream>
#include <cassert>
#include <limits>
using namespace std;

/*
  Bucket-based implementation of a open list.
  Nodes with identical heuristic value are expanded in FIFO order.
*/

template<class Entry>
TieBreakingOpenList<Entry>::TieBreakingOpenList(
	const std::vector<ScalarEvaluator *> &evals)
    : size(0), evaluators(evals) {
    lowest_bucket = std::vector<int>(dimension(), 9999999);
    last_evaluated_value.resize(evaluators.size());
}

template<class Entry>
TieBreakingOpenList<Entry>::~TieBreakingOpenList() {
}

template<class Entry>
int TieBreakingOpenList<Entry>::insert(const Entry &entry) {
    const std::vector<int> &key = last_evaluated_value;
    if(key < lowest_bucket)
        lowest_bucket = key;
    buckets[key].push_back(entry);
    size++;
    return 1;
}

template<class Entry>
Entry TieBreakingOpenList<Entry>::remove_min() {
    assert(size > 0);
    typename std::map<const std::vector<int>, Bucket>::iterator it;
    it = buckets.find(lowest_bucket); 
    assert(it != buckets.end());
    while(it->second.empty())
        it++;
    assert(it != buckets.end());
    lowest_bucket = it->first;
    size--;
    Entry result = it->second.front();
    it->second.pop_front();
    return result;
}

template<class Entry>
bool TieBreakingOpenList<Entry>::empty() const {
    return size == 0;
}

template<class Entry>
void TieBreakingOpenList<Entry>::evaluate(int g, bool preferred) {
    dead_end = false;
    dead_end_reliable = false;
    for (unsigned int i = 0; i < evaluators.size(); i++) {
        evaluators[i]->evaluate(g, preferred);

        // check for dead end
        if (evaluators[i]->is_dead_end()) {
            last_evaluated_value[i] = std::numeric_limits<int>::max();
            dead_end = true;
            if (evaluators[i]->dead_end_is_reliable()) {
                dead_end_reliable = true;
            }
        } else { // add value if no dead end
        	last_evaluated_value[i] = evaluators[i]->get_value();
		}
    }
}

template<class Entry>
bool TieBreakingOpenList<Entry>::is_dead_end() const {
    return dead_end;
}

template<class Entry>
bool TieBreakingOpenList<Entry>::dead_end_is_reliable() const {
    return dead_end_reliable;
}

template<class Entry>
const std::vector<int>& TieBreakingOpenList<Entry>::get_value() {
    return last_evaluated_value;
}

template<class Entry>
int TieBreakingOpenList<Entry>::dimension() const {
    return evaluators.size();
}

#endif

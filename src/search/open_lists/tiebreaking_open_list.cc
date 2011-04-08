// HACK! Ignore this if used as a top-level compile target.
#ifdef OPEN_LISTS_TIEBREAKING_OPEN_LIST_H

#include <iostream>
#include <cassert>
#include <limits>
#include "../scalar_evaluator.h"
#include "../option_parser.h"
using namespace std;

/*
  Bucket-based implementation of a open list.
  Nodes with identical heuristic value are expanded in FIFO order.
*/

template<class Entry>
OpenList<Entry> *TieBreakingOpenList<Entry>::_parse(OptionParser &parser) {
    parser.add_list_option<ScalarEvaluator *>("evals");
    parser.add_option<bool>(
        "pref_only", false,
        "insert only preferred operators");
    parser.add_option<bool>(
        "unsafe_pruning", true,
        "allow unsafe pruning when the main evaluator regards a state a dead end");
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new TieBreakingOpenList<Entry>(opts);
}

template<class Entry>
TieBreakingOpenList<Entry>::TieBreakingOpenList(const Options &opts)
    : OpenList<Entry>(opts.get<bool>("pref_only")),
      size(0), evaluators(opts.get_list<ScalarEvaluator *>("evals")),
      allow_unsafe_pruning(opts.get<bool>("unsafe_pruning")) {
    last_evaluated_value.resize(evaluators.size());
}

template<class Entry>
TieBreakingOpenList<Entry>::TieBreakingOpenList(
    const std::vector<ScalarEvaluator *> &evals,
    bool preferred_only, bool unsafe_pruning)
    : OpenList<Entry>(preferred_only), size(0), evaluators(evals),
      allow_unsafe_pruning(unsafe_pruning) {
    last_evaluated_value.resize(evaluators.size());
}

template<class Entry>
TieBreakingOpenList<Entry>::~TieBreakingOpenList() {
}

template<class Entry>
int TieBreakingOpenList<Entry>::insert(const Entry &entry) {
    if (OpenList<Entry>::only_preferred && !last_preferred)
        return 0;
    if (first_is_dead_end && allow_unsafe_pruning) {
        return 0;
    }
    const std::vector<int> &key = last_evaluated_value;
    buckets[key].push_back(entry);
    size++;
    return 1;
}

template<class Entry>
Entry TieBreakingOpenList<Entry>::remove_min(vector<int> *key) {
    assert(size > 0);
    typename std::map<const std::vector<int>, Bucket>::iterator it;
    it = buckets.begin();
    assert(it != buckets.end());
    assert(!it->second.empty());
    size--;
    if (key) {
        assert(key->empty());
        *key = it->first;
    }
    Entry result = it->second.front();
    it->second.pop_front();
    if (it->second.empty())
        buckets.erase(it);
    return result;
}

template<class Entry>
bool TieBreakingOpenList<Entry>::empty() const {
    return size == 0;
}

template<class Entry>
void TieBreakingOpenList<Entry>::clear() {
    buckets.clear();
    size = 0;
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
    first_is_dead_end = evaluators[0]->is_dead_end();
    last_preferred = preferred;
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
const std::vector<int> &TieBreakingOpenList<Entry>::get_value() {
    return last_evaluated_value;
}

template<class Entry>
int TieBreakingOpenList<Entry>::dimension() const {
    return evaluators.size();
}

template<class Entry>
void TieBreakingOpenList<Entry>::get_involved_heuristics(std::set<Heuristic *> &hset) {
    for (unsigned int i = 0; i < evaluators.size(); i++) {
        evaluators[i]->get_involved_heuristics(hset);
    }
}
#endif

// HACK! Ignore this if used as a top-level compile target.
#ifdef OPEN_LISTS_STANDARD_SCALAR_OPEN_LIST_H

#include "../scalar_evaluator.h"
#include "../option_parser.h"

#include <iostream>
#include <cassert>
#include <limits>

using namespace std;
template<class Entry>
OpenList<Entry> *StandardScalarOpenList<Entry>::create(
    const std::vector<string> &config, int start, int &end, bool dry_run) {
    std::vector<ScalarEvaluator *> evaluators;
    bool only_pref_ = false;
    NamedOptionParser named_option_parser;
    named_option_parser.add_bool_option("pref_only", &only_pref_,
                                        "insert only preferred operators");
    OptionParser *parser = OptionParser::instance();
    parser->parse_evals_and_options(config, start, end, evaluators,
                                    named_option_parser, true, dry_run);

    if (dry_run)
        return 0;
    else
        return new StandardScalarOpenList<Entry>(evaluators[0], only_pref_);
}

/*
  Bucket-based implementation of a open list.
  Nodes with identical heuristic value are expanded in FIFO order.
*/

template<class Entry>
StandardScalarOpenList<Entry>::StandardScalarOpenList(ScalarEvaluator *eval,
                                                      bool preferred_only)
    : OpenList<Entry>(preferred_only), size(0), evaluator(eval) {
    lowest_bucket = numeric_limits<int>::max();
}

template<class Entry>
StandardScalarOpenList<Entry>::~StandardScalarOpenList() {
}

template<class Entry>
int StandardScalarOpenList<Entry>::insert(const Entry &entry) {
    if (OpenList<Entry>::only_preferred && !last_preferred)
        return 0;
    if (dead_end) {
        return 0;
    }
    int key = last_evaluated_value;
    if (key < lowest_bucket)
        lowest_bucket = key;
    buckets[key].push_back(entry);
    size++;
    return 1;
}

template<class Entry>
Entry StandardScalarOpenList<Entry>::remove_min(vector<int> *key) {
    assert(size > 0);
    typename std::map<int, Bucket>::iterator it;
    it = buckets.find(lowest_bucket);
    assert(it != buckets.end());
    while (it->second.empty())
        ++it;
    assert(it != buckets.end());
    lowest_bucket = it->first;
    size--;
    if (key) {
        assert(key->empty());
        key->push_back(lowest_bucket);
    }
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
    lowest_bucket = numeric_limits<int>::max();
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

template<class Entry>
void StandardScalarOpenList<Entry>::get_involved_heuristics(std::set<Heuristic *>
                                                            &hset) {
    evaluator->get_involved_heuristics(hset);
}
#endif

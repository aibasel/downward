// HACK! Ignore this if used as a top-level compile target.
//#include "typed_open_list.h"
#ifdef OPEN_LISTS_TYPED_OPEN_LIST_H

#include "../option_parser.h"
#include "../globals.h"

#include <cassert>
#include <cstdlib>
using namespace std;



template<class Entry>
OpenList<Entry> *TypedOpenList<Entry>::_parse(OptionParser &parser) {
    parser.document_synopsis("Typed open list",
                             "Typed open list ....");
    parser.add_list_option<ScalarEvaluator*>("sublists", "sub evaluators");

    Options opts = parser.parse();
    if (parser.help_mode())
        return 0;

    if (opts.get_list<ScalarEvaluator*>("sublists").empty())
        parser.error("need at least one internal evaluator");
    if (parser.dry_run())
        return 0;
    else
        return new TypedOpenList<Entry>(opts);
}

template<class Entry>
TypedOpenList<Entry>::TypedOpenList(const Options &opts)
    : evaluators(opts.get_list<ScalarEvaluator *>("sublists")),
      size(0) {
}

template<class Entry>
TypedOpenList<Entry>::TypedOpenList(const vector<OpenList<Entry> *> &sublists)
    : evaluators(sublists), size(0) {
}

template<class Entry>
TypedOpenList<Entry>::~TypedOpenList() {
}

template<class Entry>
int TypedOpenList<Entry>::insert(const Entry &entry) {
    std::vector<int> key;
    for (size_t i = 0; i < evaluators.size(); i++) {
        key.push_back(evaluators[i]->get_value());
    }
    size++;
    open_list[key].push_back(entry);
    return 1;
}

template<class Entry>
Entry TypedOpenList<Entry>::remove_min(vector<int> *key) {
    assert(size > 0);

    int bucket_id = g_rng.next(open_list.size());
    typename BucketMap::iterator it = open_list.begin();
    for(int i = 0;it != open_list.end() && i < bucket_id;++i,++it);
    Bucket &bucket = it->second;

    int pos = g_rng.next(bucket.size());
    Entry result = bucket.at(pos);

//    if(key)//TODO: as this open list is only used with alt open list this doesn't make sense
//    {
//        key->insert(key->begin(),it->first.begin(), it->first.end());
//    }
    bucket.erase(bucket.begin() + pos);
    if(bucket.empty())
    {
        open_list.erase(it);
    }
    size--;
    return result;
}

template<class Entry>
bool TypedOpenList<Entry>::empty() const {
    return size == 0;
}

template<class Entry>
void TypedOpenList<Entry>::clear() {
    size = 0;
    open_list.clear();
}

template<class Entry>
void TypedOpenList<Entry>::evaluate(int g, bool preferred) {
    /*
      Treat as a dead end if
      1. at least one heuristic reliably recognizes it as a dead end, or
      2. all heuristics unreliably recognize it as a dead end
      In case 1., the dead end is reliable; in case 2. it is not.
     */

    dead_end = true;
    dead_end_reliable = false;
    for (size_t i = 0; i < evaluators.size(); i++) {
        evaluators[i]->evaluate(g, preferred);
        if (evaluators[i]->is_dead_end()) {
            if (evaluators[i]->dead_end_is_reliable()) {
                dead_end = true; // Might have been set to false.
                dead_end_reliable = true;
                break;
            }
        } else {
            dead_end = false;
        }
    }
}

template<class Entry>
bool TypedOpenList<Entry>::is_dead_end() const {
    return dead_end;
}

template<class Entry>
bool TypedOpenList<Entry>::dead_end_is_reliable() const {
    return dead_end_reliable;
}

template<class Entry>
void TypedOpenList<Entry>::get_involved_heuristics(std::set<Heuristic *> &hset) {
    for (size_t i = 0; i < evaluators.size(); i++)
        evaluators[i]->get_involved_heuristics(hset);
}

#endif

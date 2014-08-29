// HACK! Ignore this if used as a top-level compile target.
//#include "typed_open_list.h"
#ifdef OPEN_LISTS_TYPED_OPEN_LIST_H

#include "../option_parser.h"
#include "../rng.h"
#include "../globals.h"
#include "../utilities.h"

#include <cassert>
#include <cstdlib>

using namespace std;


template<class Entry>
OpenList<Entry> *TypedOpenList<Entry>::_parse(OptionParser &parser) {
    parser.document_synopsis("Typed open list",
                             "Typed open list that uses multiple evaluators to put nodes into buckets. "
                             "When retrieving a node, a bucket is chosen uniformly at random and one of the contained nodes is selected randomly. "
                             "This open list should be used in combination with other open lists, e.g. alt().");
    parser.add_list_option<ScalarEvaluator *>("sublists", "The evaluators to group the nodes by.");

    Options opts = parser.parse();
    if (parser.help_mode())
        return 0;

    opts.verify_list_non_empty<ScalarEvaluator *>("sublists");
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
    std::vector<int> key(evaluators.size());

    for (size_t i = 0; i < evaluators.size(); ++i) {
        key[i] = evaluators[i]->get_value();
    }

    typename BucketMap::iterator bucket = open_list.find(key);
    if(bucket == open_list.end()) {
        bucket_list.push_back(make_pair<vector<int>,Bucket>(key,Bucket()));
        bucket_list.back().second.push_back(entry);//TODO: c++11 list init
        open_list[key] = bucket_list.size() - 1;
    } else {
        assert(bucket->second < bucket_list.size());
        bucket_list[bucket->second].second.push_back(entry);
    }

    ++size;
    return 1;
}

template<class Entry>
Entry TypedOpenList<Entry>::remove_min(vector<int> *key) {
    assert(size > 0);

    if (key) {
        cerr << "not implemented -- see msg639 in the tracker" << endl;
        exit_with(EXIT_UNSUPPORTED);
    }

    int bucket_id = g_rng.next(bucket_list.size());
    pair<std::vector<int>,Bucket> &bucket_pair = bucket_list[bucket_id];
    vector<int> bucket_key = bucket_pair.first;//copy the key
    Bucket &bucket = bucket_pair.second;

    int pos = g_rng.next(bucket.size());
    Entry result = bucket[pos];

    bucket.erase(bucket.begin() + pos);
    if (bucket.empty()) {
        if(bucket_list.size() > 1 && (bucket_list.size() - 1 != bucket_id)) {
            //swap: move the back to the position to be deleted and then pop_back
            pair<std::vector<int>,Bucket> &last = bucket_list.back();
            bucket_list[bucket_id] = last;
            open_list[last.first] = bucket_id;
        }
        open_list.erase(bucket_key);
        bucket_list.pop_back();
    }
    --size;
    return result;
}

template<class Entry>
bool TypedOpenList<Entry>::empty() const {
    return size == 0;
}

template<class Entry>
void TypedOpenList<Entry>::clear() {
    bucket_list.clear();
    open_list.clear();
    size = 0;
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

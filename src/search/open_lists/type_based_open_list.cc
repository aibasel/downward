// HACK! Ignore this if used as a top-level compile target.
#ifdef OPEN_LISTS_TYPE_BASED_OPEN_LIST_H

#include "../globals.h"
#include "../option_parser.h"
#include "../rng.h"
#include "../utilities.h"

using namespace std;

template<class Entry>
TypeBasedOpenList<Entry>::TypeBasedOpenList(const Options &opts)
    : evaluators(opts.get_list<ScalarEvaluator *>("sublists")),
      size(0),
      dead_end(false),
      dead_end_reliable(false) {
}

template<class Entry>
TypeBasedOpenList<Entry>::~TypeBasedOpenList() {
}

template<class Entry>
int TypeBasedOpenList<Entry>::insert(const Entry &entry) {
    vector<int> key;
    key.reserve(evaluators.size());
    for (vector<ScalarEvaluator *>::iterator it = evaluators.begin(); it != evaluators.end(); ++it) {
        key.push_back((*it)->get_value());
    }

    typename KeyToBucketIndex::iterator it = key_to_bucket_index.find(key);
    if (it == key_to_bucket_index.end()) {
        keys_and_buckets.push_back(make_pair(key, Bucket()));
        keys_and_buckets.back().second.push_back(entry); // TODO: c++11 list init
        key_to_bucket_index[key] = keys_and_buckets.size() - 1;
    } else {
        size_t bucket_index = it->second;
        assert(bucket_index < keys_and_buckets.size());
        keys_and_buckets[bucket_index].second.push_back(entry);
    }

    ++size;
    return 1;
}

template<class Entry>
Entry TypeBasedOpenList<Entry>::remove_min(vector<int> *key) {
    assert(size > 0);

    if (key) {
        cerr << "not implemented -- see msg639 in the tracker" << endl;
        exit_with(EXIT_UNSUPPORTED);
    }

    int bucket_id = g_rng.next(keys_and_buckets.size());
    pair<Key, Bucket> &key_and_bucket = keys_and_buckets[bucket_id];
    Bucket &bucket = key_and_bucket.second;

    int pos = g_rng.next(bucket.size());
    Entry result = bucket[pos];

    swap_and_pop_from_vector(bucket, pos);
    if (bucket.empty()) {
        // Swap the empty bucket with the last bucket, then delete it.
        Key key_copy = key_and_bucket.first;
        Key moved_bucket_key = keys_and_buckets.back().first;
        key_to_bucket_index[moved_bucket_key] = bucket_id;
        assert(bucket_id < keys_and_buckets.size());
        swap(keys_and_buckets[bucket_id], keys_and_buckets.back());
        keys_and_buckets.pop_back();
        key_to_bucket_index.erase(key_copy);
    }
    --size;
    return result;
}

template<class Entry>
bool TypeBasedOpenList<Entry>::empty() const {
    return size == 0;
}

template<class Entry>
void TypeBasedOpenList<Entry>::clear() {
    keys_and_buckets.clear();
    key_to_bucket_index.clear();
    size = 0;
}

template<class Entry>
void TypeBasedOpenList<Entry>::evaluate(int g, bool preferred) {
    // The code is taken from AlternationOpenList
    // TODO: see issue494, common implementation of evaluate for multi evaluator open lists

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
bool TypeBasedOpenList<Entry>::is_dead_end() const {
    return dead_end;
}

template<class Entry>
bool TypeBasedOpenList<Entry>::dead_end_is_reliable() const {
    return dead_end_reliable;
}

template<class Entry>
void TypeBasedOpenList<Entry>::get_involved_heuristics(set<Heuristic *> &hset) {
    for (size_t i = 0; i < evaluators.size(); i++)
        evaluators[i]->get_involved_heuristics(hset);
}

template<class Entry>
OpenList<Entry> *TypeBasedOpenList<Entry>::_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Type-based open list",
        "Type-based open list that uses multiple evaluators to put nodes "
        "into buckets. When retrieving a node, a bucket is chosen "
        "uniformly at random and one of the contained nodes is selected "
        "uniformly randomly.");
    parser.add_list_option<ScalarEvaluator *>("sublists", "The evaluators to group the nodes by.");

    Options opts = parser.parse();
    if (parser.help_mode())
        return 0;

    opts.verify_list_non_empty<ScalarEvaluator *>("sublists");
    if (parser.dry_run())
        return 0;
    else
        return new TypeBasedOpenList<Entry>(opts);
}

#endif
